//
// Created by Dottik on 21/8/2024.
//
#include "ClosureManager.hpp"
#include <lua.h>

#include "Communication/Communication.hpp"
#include "Environment/Libraries/Globals.hpp"
#include "Logger.hpp"
#include "Luau/CodeGen.h"
#include "Luau/Compiler.h"
#include "RobloxManager.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "lapi.h"
#include "lfunc.h"
#include "lgc.h"
#include "lstring.h"
#include "lualib.h"

std::shared_ptr<ClosureManager> ClosureManager::pInstance;

std::shared_ptr<ClosureManager> ClosureManager::GetSingleton() {
    if (ClosureManager::pInstance == nullptr)
        ClosureManager::pInstance = std::make_shared<ClosureManager>();

    return ClosureManager::pInstance;
}

void ClosureManager::ResetManager(const RBX::DataModelType &resetTarget) {
    this->m_newcclosureMap[resetTarget].clear();
    this->m_hookMap[resetTarget].clear();
}

int ClosureManager::newcclosure_handler(lua_State *L) {
    const auto argc = lua_gettop(L);
    const auto manager = RobloxManager::GetSingleton();

    const auto scriptContext = manager->GetScriptContext(L);
    if (!scriptContext.has_value())
        luaL_error(L, "call resolution failed; ScriptContext unavailable");

    const auto dataModel = manager->GetDataModelFromScriptContext(scriptContext.value());
    if (!dataModel.has_value())
        luaL_error(L, "call resolution failed; DataModel unavailable");

    const auto currentDataModel = manager->GetDataModelType(dataModel.value());

    const auto clManager = ClosureManager::GetSingleton();
    if (!clManager->m_newcclosureMap[currentDataModel].contains(clvalue(L->ci->func)))
        luaL_error(L, "call resolution failed"); // using key based indexing will insert it into the map, that is wrong.

    // Due to the nature of the Lua Registry, our closures can be replaced with complete garbage.
    // Thus we must be careful, and validate that the Lua Registry ref IS present.
    Closure *closure = clManager->m_newcclosureMap[currentDataModel].at(clvalue(L->ci->func));

    luaC_threadbarrier(L);

    L->top->value.p = closure;
    L->top->tt = lua_Type::LUA_TFUNCTION;
    L->top++;

    const auto scheduler = Scheduler::GetSingleton();

    if (!RbxStu::s_mRefsMapBasedOnDataModel[currentDataModel].contains(closure))
        luaL_error(L, "call resolution failed, invalid closure state.");
    lua_getref(L, RbxStu::s_mRefsMapBasedOnDataModel[currentDataModel][closure]);
    if (lua_type(L, -1) == LUA_TFUNCTION && lua_toclosure(L, -1) == closure) {
        lua_pop(L, 1);
    } else {
        luaL_error(L, "call resolution failed, invalid ref."); // If the pointers aren't the same, and the lua type
                                                               // isn't correct either, this MUST mean this is the work
                                                               // on the devil, but we don't have a crucifix to remove
                                                               // him, so just crash.
    }
    lua_insert(L, 1);

    const auto callResult = lua_pcall(L, argc, LUA_MULTRET, 0);
    if (callResult != LUA_OK && callResult != LUA_YIELD &&
        std::strcmp(luaL_optstring(L, -1, ""), "attempt to yield across metamethod/C-call boundary") == 0) {
        return lua_yield(L, LUA_MULTRET);
    }

    if (callResult == LUA_ERRRUN)
        lua_error(L); // error string at stack top

    return lua_gettop(L);
}

bool ClosureManager::IsWrappedCClosure(lua_State *L, Closure *cl) {
    const auto manager = RobloxManager::GetSingleton();

    const auto dataModel = manager->GetDataModelFromLuaState(L);
    if (!dataModel.has_value())
        return false;

    const auto currentDataModel = manager->GetDataModelType(dataModel.value());
    return this->m_newcclosureMap[currentDataModel].contains(cl) && cl->isC && cl->c.f == newcclosure_handler;
}

bool ClosureManager::IsWrapped(lua_State *L, const Closure *closure) {
    const auto manager = RobloxManager::GetSingleton();

    const auto dataModel = manager->GetDataModelFromLuaState(L);
    if (!dataModel.has_value())
        return false;

    const auto currentDataModel = manager->GetDataModelType(dataModel.value());

    return std::ranges::any_of(this->m_newcclosureMap[currentDataModel].begin(),
                               this->m_newcclosureMap[currentDataModel].end(),
                               [&closure](const std::pair<Closure *, Closure *> cl) { return closure == cl.second; });
}

int ClosureManager::hookfunction(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    const auto logger = Logger::GetSingleton();
    const auto clManager = ClosureManager::GetSingleton();

    auto hookTarget = lua_toclosure(L, 1);
    auto hookWith = lua_toclosure(L, 2);

    if (!hookWith->isC)
        Security::GetSingleton()->WipeClosurePrototype(L, hookWith);

    clManager->FixClosure(L, hookWith); // Mark hookWith to not be collected by the lgc.

    const auto manager = RobloxManager::GetSingleton();

    const auto dataModel = manager->GetDataModelFromLuaState(L);
    if (!dataModel.has_value())
        luaL_error(L, "cannot hook function; DataModel unavailable");

    const auto currentDataModel = manager->GetDataModelType(dataModel.value());

    /*
     *  Supported hooks:
     *  - C->C
     *  - C->NC
     *  - C->L
     *  - NC->C
     *  - NC->L
     *  - NC->NC
     *  - L->C
     *  - L->L
     *  - L->NC
     */

    // C->C/L
    if (hookTarget->isC && !clManager->IsWrappedCClosure(L, hookTarget)) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        const auto cl = lua_toclosure(L, -1);

        // Freeze hookedCl's lua_CFunction to avoid breaking due to upvalues.
        hookTarget->c.f = [](lua_State *L) { return 0; };

        // If hookWith nups > hookWhat nups we must wrap hookWith on a new c closure to allow the operation to happen,
        // else, this will not work! Even then, this allows us to support L closure hooking!

        auto wrappedHookWith = lua_toclosure(L, -2);
        if (!hookWith->isC || clManager->IsWrappedCClosure(L, hookWith) ||
            hookTarget->nupvalues > wrappedHookWith->nupvalues) {
            lua_pushvalue(L, 2);
            ClosureManager::newcclosure(L);
            wrappedHookWith = lua_toclosure(L, -1);

            if (clManager->IsWrappedCClosure(L, hookWith))
                clManager->m_newcclosureMap[currentDataModel][hookTarget] =
                        clManager->m_newcclosureMap[currentDataModel][hookWith];

            if (!hookWith->isC)
                clManager->m_newcclosureMap[currentDataModel][hookTarget] =
                        clManager->m_newcclosureMap[currentDataModel][wrappedHookWith];
        }

        for (int i = 0; i < wrappedHookWith->nupvalues; i++)
            setobj2n(L, &hookTarget->c.upvals[i], &wrappedHookWith->c.upvals[i]);

        hookTarget->nupvalues = wrappedHookWith->nupvalues;
        hookTarget->c.f = wrappedHookWith->c.f;
        hookTarget->c.cont = wrappedHookWith->c.cont;

        L->top->value.p = cl;
        L->top->tt = LUA_TFUNCTION;
        L->top++;
        return 1;
    }

    // NC->C/L || Simple pointer substitution
    if (clManager->IsWrappedCClosure(L, hookTarget) && !clManager->IsWrappedCClosure(L, hookWith)) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        const auto cl = lua_toclosure(L, -1);

        clManager->m_newcclosureMap[currentDataModel][cl] =
                clManager->m_newcclosureMap[currentDataModel]
                                           [hookTarget]; // We must substitute the refs on original to point to the new,
                                                         // but we must keep the clones'.

        clManager->m_newcclosureMap[currentDataModel][hookTarget] = hookWith;
        L->top->value.p = cl;
        L->top->tt = LUA_TFUNCTION;
        L->top++;
        return 1;
    }


    // NC->NC || Simple pointer substitution
    if (clManager->IsWrappedCClosure(L, hookTarget) && clManager->IsWrappedCClosure(L, hookWith)) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        const auto cl = lua_toclosure(L, -1);

        clManager->m_newcclosureMap[currentDataModel][cl] =
                clManager->m_newcclosureMap[currentDataModel]
                                           [hookTarget]; // We must substitute the refs on original to point to the new,
        // but we must keep the clones'.

        clManager->m_newcclosureMap[currentDataModel][hookTarget] =
                clManager->m_newcclosureMap[currentDataModel]
                                           [hookWith]; // Copy the ref of hookWith to be used with hookWhat.

        L->top->value.p = cl;
        L->top->tt = LUA_TFUNCTION;
        L->top++;
        return 1;
    }


    // L->C/NC/L
    if (!hookTarget->isC) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        const auto cl = lua_toclosure(L, -1);
        lua_pop(L, 1);

        auto wrappedHookWith = hookWith;

        if (hookWith->isC) {
            lua_pushvalue(L, 2);
            ClosureManager::newlclosure(L);
            wrappedHookWith = lua_toclosure(L, -1); // Wrap NC/C into L for hooking.
            clManager->FixClosure(L, wrappedHookWith);
        }

        hookTarget->env = wrappedHookWith->env;
        hookTarget->stacksize = wrappedHookWith->stacksize;
        hookTarget->preload = wrappedHookWith->preload;

        for (int i = 0; i < wrappedHookWith->nupvalues; i++)
            setobj2n(L, &hookTarget->l.uprefs[i], &wrappedHookWith->l.uprefs[i]);

        hookTarget->nupvalues = wrappedHookWith->nupvalues;
        hookTarget->l.p = wrappedHookWith->l.p;

        L->top->value.p = cl;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        return 1;
    }

    luaL_error(L, "hookfunction: not implemented");
}

int ClosureManager::unhookfunction(lua_State *L) {
    luaL_checktype(L, -1, LUA_TFUNCTION);
    const auto manager = RobloxManager::GetSingleton();

    const auto dataModel = manager->GetDataModelFromLuaState(L);
    if (!dataModel.has_value())
        luaL_error(L, "cannot unhook function; DataModel unavailable");

    auto dataModelType = manager->GetDataModelType(dataModel.value());

    const auto clManager = ClosureManager::GetSingleton();
    const auto hookedCl = lua_toclosure(L, -1);
    if (!clManager->m_hookMap[dataModelType].contains(hookedCl))
        luaL_argerrorL(L, 1, "The given closure has not been hooked");

    const auto hookInfo = clManager->m_hookMap[dataModelType][hookedCl];

    if (hookInfo.isWrappedHook)
        luaL_argerrorL(L, 1, "Unhooking C->L or L->C hooks is not implemented yet!");

    const auto originalCl = hookInfo.originalClosureClone;

    if (hookedCl->isC) {
        // Freeze hookedCl's lua_CFunction to avoid breaking due to upvalues.
        hookedCl->c.f = [](lua_State *L) { return 0; };
        hookedCl->c.cont = [](lua_State *L, int status) { return 0; };

        for (int i = 0; i < originalCl->nupvalues; i++)
            setobj2n(L, &hookedCl->c.upvals[i], &originalCl->c.upvals[i]);

        hookedCl->nupvalues = originalCl->nupvalues;
        hookedCl->c.f = originalCl->c.f;
        hookedCl->c.cont = originalCl->c.cont;
    } else {
        hookedCl->env = originalCl->env;
        hookedCl->stacksize = originalCl->stacksize;
        hookedCl->preload = originalCl->preload;

        for (int i = 0; i < originalCl->nupvalues; i++)
            setobj2n(L, &hookedCl->l.uprefs[i], &originalCl->l.uprefs[i]);

        hookedCl->nupvalues = originalCl->nupvalues;
        hookedCl->l.p = originalCl->l.p;
    }

    hookedCl->env = originalCl->env;

    return 0;
}

void ClosureManager::FixClosure(lua_State *L, Closure *closure) {
    L->top->value.p = closure;
    L->top->tt = LUA_TFUNCTION;
    L->top++;
    const auto manager = RobloxManager::GetSingleton();

    const auto scriptContext = manager->GetScriptContext(L);
    if (!scriptContext.has_value())
        luaL_error(L, "cannot fix closure in memory; ScriptContext unavailable");

    const auto dataModel = manager->GetDataModelFromScriptContext(scriptContext.value());
    if (!dataModel.has_value())
        luaL_error(L, "cannot fix closure in memory; DataModel unavailable");

    const auto currentDataModel = manager->GetDataModelType(dataModel.value());

    if (!RbxStu::s_mRefsMapBasedOnDataModel[currentDataModel].contains(closure))
        RbxStu::s_mRefsMapBasedOnDataModel[currentDataModel][closure] = lua_ref(L, -1);
    lua_pop(L, 1);
    // Push to the ref list.
}

int ClosureManager::newcclosure(lua_State *L) {
    // Using relative offsets, since this function may be called by other functions! (hookfunc)
    auto closureIndex = -1;
    if (lua_type(L, -1) == LUA_TSTRING)
        closureIndex--;

    luaL_checktype(L, closureIndex, LUA_TFUNCTION);
    const char *functionName = nullptr;
    if (lua_type(L, -1) == LUA_TSTRING)
        functionName = luaL_optstring(L, -1, nullptr);

    const auto manager = RobloxManager::GetSingleton();

    const auto dataModel = manager->GetDataModelFromLuaState(L);
    if (!dataModel.has_value())
        luaL_error(L, "cannot wrap closure; DataModel unavailable");

    const auto currentDataModel = manager->GetDataModelType(dataModel.value());

    const auto clManager = ClosureManager::GetSingleton();
    const auto closure = lua_toclosure(L, closureIndex);
    clManager->FixClosure(L, closure);
    lua_pushcclosurek(L, ClosureManager::newcclosure_handler, functionName, 0, nullptr);
    const auto cclosure = lua_toclosure(L, closureIndex);
    clManager->m_newcclosureMap[currentDataModel][cclosure] = closure;
    lua_remove(L, lua_gettop(L) - 1); // Balance lua stack.
    return 1;
}

int ClosureManager::newlclosure(lua_State *L) {
    luaL_checktype(L, -1, LUA_TFUNCTION);
    ClosureManager::GetSingleton()->FixClosure(L, lua_toclosure(L, -1));
    lua_newtable(L); // t
    lua_newtable(L); // Meta

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfield(L, -2, "__index");
    lua_setreadonly(L, -1, true);
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "abcdefg"); // Set abcdefg to that of idx
    auto code = "return abcdefg(...)";

    constexpr auto compileOpts = Luau::CompileOptions{2, 2};
    const auto bytecode = Luau::compile(code, compileOpts);
    luau_load(L, "lclosurewrapper", bytecode.c_str(), bytecode.size(), -1);

    if (Communication::GetSingleton()->IsCodeGenerationEnabled()) {
        const Luau::CodeGen::CompilationOptions opts{0};
        Logger::GetSingleton()->PrintInformation(
                RbxStu::Anonymous, "Native Code Generation is enabled! Compiling Luau Bytecode -> Native");
        Luau::CodeGen::compile(L, -1, opts);
    }

    lua_remove(L, lua_gettop(L) - 1); // Balance lua stack.
    return 1;
}

int ClosureManager::clonefunction(lua_State *L) {
    luaL_checktype(L, -1, LUA_TFUNCTION);

    const auto manager = RobloxManager::GetSingleton();

    const auto dataModel = manager->GetDataModelFromLuaState(L);
    if (!dataModel.has_value())
        luaL_error(L, "cannot clone closure; DataModel unavailable");

    auto dataModelType = manager->GetDataModelType(dataModel.value());


    if (const auto originalCl = lua_toclosure(L, -1); originalCl->isC) {
        const auto clManager = ClosureManager::GetSingleton();
        Closure *newcl = luaF_newCclosure(L, originalCl->nupvalues, originalCl->env);

        if (originalCl->c.debugname != nullptr)
            newcl->c.debugname = originalCl->c.debugname;

        for (int i = 0; i < originalCl->nupvalues; i++)
            setobj2n(L, &newcl->c.upvals[i], &originalCl->c.upvals[i]);

        newcl->c.f = originalCl->c.f;
        newcl->c.cont = originalCl->c.cont;
        setclvalue(L, L->top, newcl);
        L->top++;
        ClosureManager::GetSingleton()->FixClosure(L, lua_toclosure(L, -1));

        // Newcclosures may wrap C closures. Thus, we must obtain the original and bind it to what it used to be, as you
        // want the call to redirect correctly! Allow newcclosures to be cloned successfully by cloning the original for
        // the wrapper.
        if (clManager->m_newcclosureMap[dataModelType].contains(originalCl))
            clManager->m_newcclosureMap[dataModelType][newcl] =
                    clManager->m_newcclosureMap[dataModelType][originalCl]; // Redirect to the correct Registry ref.

        lua_remove(L, lua_gettop(L) - 1); // Balance lua stack.
        return 1;
    } else {
        setclvalue(L, L->top, originalCl) L->top++;
        lua_clonefunction(L, -1);
        ClosureManager::GetSingleton()->FixClosure(L, lua_toclosure(L, -1));
        lua_remove(L, lua_gettop(L) - 1);

        Security::GetSingleton()->SetLuaClosureSecurity(
                lua_toclosure(L, -1), static_cast<RBX::Lua::ExtraSpace *>(L->userdata)->contextInformation.identity);

        lua_remove(L, lua_gettop(L) - 1); // Balance lua stack.
        return 1;
    }
}
