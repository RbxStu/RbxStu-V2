//
// Created by Dottik on 21/8/2024.
//
#include "ClosureManager.hpp"
#include <lua.h>

#include "Communication.hpp"
#include "Logger.hpp"
#include "Luau/CodeGen.h"
#include "Luau/Compiler.h"
#include "Security.hpp"
#include "lapi.h"
#include "lfunc.h"
#include "lgc.h"
#include "lualib.h"

std::shared_ptr<ClosureManager> ClosureManager::pInstance;

std::shared_ptr<ClosureManager> ClosureManager::GetSingleton() {
    if (ClosureManager::pInstance == nullptr)
        ClosureManager::pInstance = std::make_shared<ClosureManager>();

    return ClosureManager::pInstance;
}

int ClosureManager::newcclosure_handler(lua_State *L) {

    const auto argc = lua_gettop(L);
    const auto clManager = ClosureManager::GetSingleton();
    const int closureRef = clManager->m_newcclosureMap.contains(clvalue(L->ci->func));

    lua_getref(L, closureRef);
    const auto realClosure = lua_toclosure(L, -1);
    if (realClosure == nullptr)
        return 0;

    luaC_threadbarrier(L);
    lua_pop(L, 1);
    L->top->value.p = realClosure;
    L->top->tt = LUA_TFUNCTION;
    L->top++; // Increase top

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

bool ClosureManager::IsWrappedCClosure(Closure *cl) const {
    return this->m_newcclosureMap.contains(cl) || !cl->isC || cl->c.f == newcclosure_handler;
}

int ClosureManager::hookfunction(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    const auto clManager = ClosureManager::GetSingleton();

    auto hookWhat = lua_toclosure(L, 1);
    auto hookWith = lua_toclosure(L, 2);

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

    // C->C/NC/L
    if (hookWhat->isC && !clManager->IsWrappedCClosure(hookWhat)) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        lua_remove(L, -2);

        // Freeze hookedCl's lua_CFunction to avoid breaking due to upvalues.
        hookWhat->c.f = [](lua_State *L) { return 0; };
        hookWhat->c.cont = [](lua_State *L, int status) { return 0; };

        // If hookWith nups > hookWhat nups we must wrap hookWith on a new c closure to allow the operation to happen,
        // else, this will not work! Even then, this allows us to support L closure hooking!

        lua_pushvalue(L, 2);
        ClosureManager::newcclosure(L);
        lua_remove(L, -2);

        // ReSharper disable once CppDeclarationHidesLocal
        const auto hookWith = lua_toclosure(L, -1);
        lua_pushvalue(L, -2); // Push cloned original again.

        for (int i = 0; i < hookWith->nupvalues; i++)
            setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);

        hookWhat->nupvalues = hookWith->nupvalues;
        hookWhat->c.f = hookWith->c.f;
        hookWhat->c.cont = hookWith->c.cont;

        return 1;
    }

    // NC->C/L || Simple pointer substitution
    if (clManager->IsWrappedCClosure(hookWhat) && !clManager->IsWrappedCClosure(hookWith)) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        lua_remove(L, -2);

        clManager->m_newcclosureMap[lua_toclosure(L, -1)] =
                clManager->m_newcclosureMap[hookWhat]; // We must substitute the refs on original to point to the new,
                                                       // but we must keep the clones'.

        clManager->m_newcclosureMap[hookWhat] = lua_ref(L, 2);

        return 1;
    }


    // NC->NC || Simple pointer substitution
    if (clManager->IsWrappedCClosure(hookWhat) && clManager->IsWrappedCClosure(hookWith)) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        lua_remove(L, -2);

        clManager->m_newcclosureMap[lua_toclosure(L, -1)] =
                clManager->m_newcclosureMap[hookWhat]; // We must substitute the refs on original to point to the new,
        // but we must keep the clones'.

        clManager->m_newcclosureMap[hookWhat] =
                clManager->m_newcclosureMap[hookWith]; // Copy the ref of hookWith to be used with hookWhat.

        return 1;
    }

    if (!hookWhat->isC) {
        lua_pushvalue(L, 1);
        ClosureManager::clonefunction(L);
        lua_remove(L, -2);

        if (hookWith->isC ||
            hookWith->nupvalues > hookWhat->nupvalues) { // Bypass upvalue limit for L closures, wrap NC/C.
            lua_pushvalue(L, 2);
            ClosureManager::newlclosure(L);
            lua_remove(L, -2);
            hookWith = lua_toclosure(L, -1); // Wrap NC/C into L for hooking.
        }

        hookWhat->env = hookWith->env;
        hookWhat->stacksize = hookWith->stacksize;
        hookWhat->preload = hookWith->preload;

        for (int i = 0; i < hookWith->nupvalues; i++)
            setobj2n(L, &hookWhat->l.uprefs[i], &hookWith->l.uprefs[i]);

        hookWhat->nupvalues = hookWith->nupvalues;
        hookWhat->l.p = hookWhat->l.p;

        return 1;
    }

    return 1;
}

int ClosureManager::unhookfunction(lua_State *L) {
    luaL_checktype(L, -1, LUA_TFUNCTION);
    const auto clManager = ClosureManager::GetSingleton();
    const auto hookedCl = lua_toclosure(L, -1);
    if (!clManager->m_hookMap.contains(hookedCl))
        luaL_argerrorL(L, 1, "The given closure has not been hooked");

    const auto hookInfo = clManager->m_hookMap[hookedCl];

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

int ClosureManager::newcclosure(lua_State *L) {
    // Using relative offsets, since this function may be called by other functions! (hookfunc)
    luaL_checktype(L, -1, LUA_TFUNCTION);
    const auto clManager = ClosureManager::GetSingleton();
    const auto clReference = lua_ref(L, -1);
    lua_pushcclosurek(L, ClosureManager::newcclosure_handler, nullptr, 0, nullptr);
    const auto cclosure = lua_toclosure(L, -1);
    clManager->m_newcclosureMap[cclosure] = clReference;
    lua_remove(L, -2); // Balance lua stack.
    return 1;
}

int ClosureManager::newlclosure(lua_State *L) {
    luaL_checktype(L, -1, LUA_TFUNCTION);
    const auto clIndex = lua_gettop(L);
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
    auto bytecode = Luau::compile(code, compileOpts);
    luau_load(L, "lclosurewrapper", bytecode.c_str(), bytecode.size(), -1);

    if (Communication::GetSingleton()->IsCodeGenerationEnabled()) {
        const Luau::CodeGen::CompilationOptions opts{0};
        Logger::GetSingleton()->PrintInformation(
                RbxStu::Anonymous, "Native Code Generation is enabled! Compiling Luau Bytecode -> Native");
        Luau::CodeGen::compile(L, -1, opts);
    }

    lua_ref(L, -1); // Avoid collection.
    lua_remove(L, -2);
    return 1;
}

int ClosureManager::clonefunction(lua_State *L) {
    luaL_checktype(L, -1, LUA_TFUNCTION);

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

        // Newcclosures may wrap C closures. Thus, we must obtain the original and bind it to what it used to be, as you
        // want the call to redirect correctly! Allow newcclosures to be cloned successfully by cloning the original for
        // the wrapper.
        if (clManager->m_newcclosureMap.contains(originalCl))
            clManager->m_newcclosureMap[newcl] =
                    clManager->m_newcclosureMap[originalCl]; // Redirect to the correct Registry ref.

        return 1;
    } else {
        setclvalue(L, L->top, originalCl) L->top++;
        lua_clonefunction(L, -1);

        Security::GetSingleton()->SetLuaClosureSecurity(lua_toclosure(L, -1),
                                                        static_cast<RBX::Lua::ExtraSpace *>(L->userdata)->identity);

        return 1;
    }
}
