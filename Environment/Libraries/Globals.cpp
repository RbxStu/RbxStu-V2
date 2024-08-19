//
// Created by roale on 18/8/2024.
//

#include "Globals.hpp"

#include <HttpStatus.hpp>

#include "RobloxManager.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "cpr/api.h"
#include "lgc.h"
#include "lmem.h"

namespace RbxStu {
    int getrawmetatable(lua_State *L) {
        luaL_checkany(L, 1);

        if (!lua_getmetatable(L, 1))
            lua_pushnil(L);

        return 1;
    }

    int islclosure(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TFUNCTION);

        lua_pushboolean(L, !lua_iscfunction(L, 1));
        return 1;
    }

    int iscclosure(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TFUNCTION);

        lua_pushboolean(L, lua_iscfunction(L, 1));
        return 1;
    }

    int getreg(lua_State *L) {
        lua_pushvalue(L, LUA_REGISTRYINDEX);
        return 1;
    }

    int getrenv(lua_State *L) {
        const auto scheduler = Scheduler::GetSingleton();
        lua_pushvalue(scheduler->GetGlobalRobloxState().value(), LUA_REGISTRYINDEX);
        lua_xmove(scheduler->GetGlobalRobloxState().value(), L, 1);
        return 1;
    }

    int getsenv(lua_State *L) { // nigger implement err
        lua_pushvalue(Scheduler::GetSingleton()->GetGlobalExecutorState().value(), LUA_REGISTRYINDEX);
        lua_xmove(L, L, 1);
        return 1;
    }

    int getgenv(lua_State *L) {
        const auto scheduler = Scheduler::GetSingleton();
        lua_pushvalue(scheduler->GetGlobalExecutorState().value(), LUA_REGISTRYINDEX);
        lua_xmove(scheduler->GetGlobalExecutorState().value(), L, 1);
        return 1;
    }

    int getnamecallmethod(lua_State *L) {
        const auto szNamecall = lua_namecallatom(L, nullptr);

        if (szNamecall == nullptr) {
            lua_pushnil(L);
        } else {
            lua_pushstring(L, szNamecall);
        }

        return 1;
    }

    int getgc(lua_State *L) {
        const bool addTables = luaL_optboolean(L, 1, false);
        lua_newtable(L);

        typedef struct {
            lua_State *pLua;
            bool accessTables;
            int itemsFound;
        } GCOContext;

        auto gcCtx = GCOContext{L, addTables, 0};

        const auto ullOldThreshold = L->global->GCthreshold;
        L->global->GCthreshold = SIZE_MAX;
        // Never return true. We aren't deleting shit.
        luaM_visitgco(L, &gcCtx, [](void *ctx, lua_Page *pPage, GCObject *pGcObj) -> bool {
            const auto pCtx = static_cast<GCOContext *>(ctx);
            const auto ctxL = pCtx->pLua;

            if (iswhite(pGcObj))
                return false; // The object is being collected/checked. Skip it.

            if (const auto gcObjType = pGcObj->gch.tt; gcObjType == LUA_TFUNCTION || gcObjType == LUA_TUSERDATA ||
                                                       gcObjType == LUA_TBUFFER || gcObjType == LUA_TLIGHTUSERDATA ||
                                                       gcObjType == LUA_TTABLE && pCtx->accessTables) {
                // Push copy to top of stack.
                ctxL->top->value.gc = pGcObj;
                ctxL->top->tt = gcObjType;
                ctxL->top++;

                // Store onto GC table.
                const auto tIndx = pCtx->itemsFound++;
                lua_rawseti(ctxL, -2, tIndx + 1);
            }
            return false;
        });
        L->global->GCthreshold = ullOldThreshold;

        return 1;
    }

    int setreadonly(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        const bool bIsReadOnly = luaL_optboolean(L, 2, false);
        lua_setreadonly(L, 1, bIsReadOnly);

        return 0;
    }

    int isreadonly(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        lua_pushboolean(L, lua_getreadonly(L, 1));
        return 1;
    }

    int isluau(lua_State *L) {
        lua_pushboolean(L, true);
        return 1;
    }

    int isrbxactive(lua_State *L) {
        lua_pushboolean(L, RobloxManager::GetSingleton()->IsActive());
        return 1;
    }

    int gethui(lua_State *L) {
        // who cares about this func
        lua_getglobal(L, "game");
        lua_getfield(L, 1, "GetService");
        lua_pushvalue(L, 1);
        lua_pushstring(L, "CoreGui");
        lua_call(L, 2, 1);
        lua_getglobal(L, "cloneref");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        lua_getfield(L, -1, "RobloxGui");
        lua_getglobal(L, "cloneref");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);

        return 1;
    }

    int httpget(lua_State *L) {
        const std::string url = luaL_checkstring(L, 1);

        if (url.find("http://") == std::string::npos && url.find("https://") == std::string::npos)
            luaG_runerror(L, "Invalid protocol (expected 'http://' or 'https://')");

        const auto scheduler = Scheduler::GetSingleton();

        scheduler->ScheduleJob(SchedulerJob(
                L, [](lua_State *L, std::shared_future<std::function<int(lua_State *)>> *callbackToExecute) {
                    *callbackToExecute = std::async(std::launch::async, [L]() -> std::function<int(lua_State *)> {
                        const auto response =
                                cpr::Get(cpr::Url{lua_tostring(L, 1)}, cpr::Header{{"User-Agent", "Roblox/WinInet"}});

                        auto output = std::string("");

                        if (HttpStatus::IsError(response.status_code)) {
                            output = std::format("HttpGet failed\nResponse {} - {}. {}",
                                                 std::to_string(response.status_code),
                                                 HttpStatus::ReasonPhrase(response.status_code),
                                                 std::string(response.error.message));
                        } else {
                            output = response.text;
                        }

                        return [output](lua_State *L) -> int {
                            lua_pushlstring(L, output.c_str(), output.size());
                            return 1;
                        };
                    });
                }));
        return lua_yield(L, 1);
    }

    int checkcaller(lua_State *L) {
        // Our check caller implementation, is, in fact, quite simple!
        // We leave the 63d bit on the capabilities set. Then we retrieve it AND it, receiving the expected, if it
        // doesn't match, then, we are 100% not in our thread.
        lua_pushboolean(L, Security::GetSingleton()->IsOurThread(L));
        return 1;
    }

    int setrawmetatable(lua_State *L) {
        luaL_argexpected(L,
                         lua_istable(L, 1) || lua_islightuserdata(L, 1) || lua_isuserdata(L, 1) || lua_isbuffer(L, 1) ||
                                 lua_isvector(L, 1),
                         1, "table or userdata or vector or buffer");

        luaL_checktype(L, 2, lua_Type::LUA_TTABLE);

        lua_setmetatable(L, 1);
        return 0;
    }

    int setnamecallmethod(lua_State *L) {
        luaL_checkstring(L, 1);
        if (L->namecall != nullptr)
            L->namecall = &L->top->value.gc->ts;

        return 0;
    }

    int compareinstances(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TUSERDATA);
        luaL_checktype(L, 2, lua_Type::LUA_TUSERDATA);

        lua_pushboolean(L, *static_cast<const std::uintptr_t *>(lua_touserdata(L, 1)) ==
                                   *static_cast<const std::uintptr_t *>(lua_touserdata(L, 2)));

        return 1;
    }

    int fireproximityprompt(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TUSERDATA);

        const auto proximityPrompt = *static_cast<std::uintptr_t **>(lua_touserdata(L, 1));
        reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ProximityPrompt_onTriggered>(
                RobloxManager::GetSingleton()->GetRobloxFunction("RBX::ProximityPrompt::onTriggerd"))(proximityPrompt);
        return 0;
    }

    int cloneref(lua_State *L) { // askdjhasbdgaigdsais
    luaL_checktype(L, 1, LUA_TUSERDATA);
    auto userdata = *static_cast<void **>(lua_touserdata(L, 1));
    auto robloxManager = RobloxManager::GetSingleton();
    auto pushInstanceFunc = robloxManager->GetRobloxFunction("RBX::Instance::pushInstance");

    lua_pushlightuserdata(L, pushInstanceFunc);
    lua_rawget(L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(L, userdata);
    lua_rawget(L, -2);

    if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, -1);
    } else {
        reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_Instance_pushInstance>(pushInstanceFunc)(L, userdata);
        lua_pushlightuserdata(L, userdata);
        lua_pushvalue(L, -3);
        lua_rawset(L, -4);
    }
    return 1;
    }


} // namespace RbxStu


// forgot about this shit
std::string Globals::GetLibraryName() const { return "rbxstu"; }
std::int32_t Globals::GetFunctionCount() const { return 0; }
luaL_Reg *Globals::GetLibraryFunctions() const {
    auto *reg = new luaL_Reg[]{{"getrawmetatable", RbxStu::getrawmetatable},
                               {"iscclosure", RbxStu::iscclosure},
                               {"islclosure", RbxStu::islclosure},
                               {"getreg", RbxStu::getreg},
                               {"getgenv", RbxStu::getgenv},
                               {"getrenv", RbxStu::getrenv},
                               {"getnamecallmethod", RbxStu::getnamecallmethod},
                               {"getgc", RbxStu::getgc},
                               {"setreadonly", RbxStu::setreadonly},
                               {"isreadonly", RbxStu::isreadonly},
                               {"isluau", RbxStu::isluau},
                               {"httpget", RbxStu::httpget},
                               {"gethui", RbxStu::gethui},
                               {"checkcaller", RbxStu::checkcaller},
                               {"setrawmetatable", RbxStu::setrawmetatable},
                               {"compareinstances", RbxStu::compareinstances},
                               {"fireproximityprompt", RbxStu::fireproximityprompt},
                               {"cloneref", RbxStu::cloneref},
                               {"getsenv", RbxStu::getsenv},
                               {"isrbxactive", RbxStu::isrbxactive},
                               {nullptr, nullptr}};
    return reg;
}
