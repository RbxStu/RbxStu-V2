//
// Created by Pixeluted on 22/08/2024.
//

#include "Script.hpp"

#include "ClosureManager.hpp"
#include "Luau/Compiler.h"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "lgc.h"
#include "lmem.h"

namespace RbxStu {
    namespace Script {
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

                if (isdead(ctxL->global, pGcObj))
                    return false; // The object is being collected/checked. Skip it.

                if (const auto gcObjType = pGcObj->gch.tt;
                    gcObjType == LUA_TFUNCTION || gcObjType == LUA_TUSERDATA || gcObjType == LUA_TBUFFER ||
                    gcObjType == LUA_TLIGHTUSERDATA || gcObjType == LUA_TTABLE && pCtx->accessTables) {
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

        int getgenv(lua_State *L) {
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            return 1;
        }

        int getrenv(lua_State *L) {
            // Utilities::RobloxThreadSuspension threadSuspension(true);
            const auto rL = Scheduler::GetSingleton()->GetGlobalRobloxState().value();
            lua_pushvalue(rL, LUA_GLOBALSINDEX);
            lua_xmove(rL, L, 1);
            return 1;
        }

        int gettenv(lua_State *L) {
            luaL_checktype(L, 1, LUA_TTHREAD);
            const auto th = lua_tothread(L, 1);
            lua_pushvalue(th, LUA_GLOBALSINDEX);
            lua_xmove(th, L, 1);

            return 1;
        }

        int getreg(lua_State *L) {
            lua_pushvalue(L, LUA_REGISTRYINDEX);
            return 1;
        }

        int setidentity(lua_State *L) {
            luaL_checkinteger(L, 1);
            auto newIdentity = lua_tointeger(L, 1);
            const auto security = Security::GetSingleton();
            security->SetThreadSecurity(L, newIdentity);

            // WARNING: Doing this will break metamethod hooks and produces undefine behaviour on which are nested
            // inside of others! If calling closure is LClosure and is not base CI, set its capabilities too if
            // (L->base_ci != L->ci) {
            //     auto callingClosure = (L->ci - 1)->func->value.gc->cl;
            //     if (!callingClosure.isC) {
            //         security->SetLuaClosureSecurity(&callingClosure, newIdentity);
            //     }
            // }

            // Capabilities and identity are applied next resumption cycle, we need to yield!
            const auto scheduler = Scheduler::GetSingleton();
            scheduler->ScheduleJob(SchedulerJob(
                    L, [](lua_State *L, std::shared_future<std::function<int(lua_State *)>> *callbackToExecute) {
                        *callbackToExecute = std::async(std::launch::async, [L]() -> std::function<int(lua_State *)> {
                            Sleep(16); // FIXME: Crashes with "bad function call"!
                            return [](lua_State *L) { return 0; };
                        });
                    }));


            return lua_yield(L, 0);
        }

        int getidentity(lua_State *L) {
            auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
            lua_pushinteger(L, plStateUd->contextInformation.identity);
            return 1;
        }

        int printcaps(lua_State *L) {
            auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
            const auto security = Security::GetSingleton();
            security->PrintCapabilities(plStateUd->capabilities);
            return 0;
        }

        int getscriptbytecode(lua_State *L) {
            Utilities::checkInstance(L, 1, "LuaSourceContainer");
            auto scriptInstance = *static_cast<void**>(lua_touserdata(L, 1));
            auto scriptSourceDataOffset = 0x0;
            auto doesOffsetHaveSource = true;

            /**
             * In studio, LocalScripts have source and ModuleScripts have bytecode of the code, don't know why is there his mess
             * In addition, both ModuleScript and LocalScript have different offsets for this data, average roblox mess
             *
             * So for LocalScripts, we have to compile the bytecode ourself, and for ModuleScripts we can return the bytecode
             * And for more fun, sometimes the scripts dont have any source/bytecode, fun.
             */

            auto logger = Logger::GetSingleton();

            lua_getfield(L, 1, "ClassName");
            if (const auto className = lua_tostring(L, -1); strcmp(className, "LocalScript") == 0) {
                logger->PrintDebug(RbxStu::Anonymous, "Using local script offset");
                scriptSourceDataOffset = 0x1C0;
            } else if (strcmp(className, "ModuleScript") == 0) {
                logger->PrintDebug(RbxStu::Anonymous, "Using module script offset");
                scriptSourceDataOffset = 0x378;
                doesOffsetHaveSource = false;
            }

            if (!Utilities::IsPointerValid<void*>(static_cast<void **>(
                        reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(scriptInstance) + scriptSourceDataOffset)))) {
                luaG_runerrorL(L, "RbxStu V2 might have out of date offsets!");
            }

            auto scriptSourceData = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(scriptInstance) + scriptSourceDataOffset);
            auto scriptSource = *reinterpret_cast<const char**>(reinterpret_cast<uintptr_t>(scriptSourceData) + 0x10);
            if (scriptSource == nullptr) {
                luaG_runerrorL(L, "Roblox mess have fucked up, please complain to bitdancer.");
            }

            logger->PrintDebug(RbxStu::Anonymous, std::format("Got source: {}", scriptSource));

            if (doesOffsetHaveSource) {
                auto opts = Luau::CompileOptions{};
                opts.debugLevel = 2;
                opts.optimizationLevel = 2;

                const auto bytecode = Luau::compile(scriptSource, opts);
                lua_pushstring(L, bytecode.c_str());
                return 1;
            } else {
                lua_pushstring(L, scriptSource);
                return 1;
            }
        }

        int checkcallstack(lua_State *L) {
            if (!Security::GetSingleton()->IsOurThread(L)) {
                lua_pushboolean(L, false);
                return 1;
            }

            auto currentCi = L->ci;
            while (L->base_ci < currentCi) {
                if (const auto pClosure = static_cast<Closure *>(currentCi->func->value.p); pClosure->isC) {
                    if (pClosure->c.debugname != nullptr &&
                        !ClosureManager::GetSingleton()->IsWrappedCClosure(L, pClosure)) {
                        lua_pushboolean(L, false);
                        return 1;
                    }
                } else {
                    if (pClosure->l.p->linedefined != -1) {
                        lua_pushboolean(L, false);
                        return 1;
                    }
                }

                currentCi--;
            }

            lua_pushboolean(L, true);
            return 1;
        }

    } // namespace Script
} // namespace RbxStu

std::string Script::GetLibraryName() { return "scriptlib"; }
luaL_Reg *Script::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"getgc", RbxStu::Script::getgc},
                                    {"getgenv", RbxStu::Script::getgenv},
                                    {"getrenv", RbxStu::Script::getrenv},
                                    {"getreg", RbxStu::Script::getreg},
                                    {"gettenv", RbxStu::Script::gettenv},

                                    {"getidentity", RbxStu::Script::getidentity},
                                    {"getthreadidentity", RbxStu::Script::getidentity},
                                    {"getthreadcontext", RbxStu::Script::getidentity},
                                    {"getscriptbytecode", RbxStu::Script::getscriptbytecode},

                                    {"setidentity", RbxStu::Script::setidentity},
                                    {"setthreadcontext", RbxStu::Script::setidentity},
                                    {"setthreadidentity", RbxStu::Script::setidentity},

                                    {"checkcallstack", RbxStu::Script::checkcallstack},

                                    {"printcaps", RbxStu::Script::printcaps},

                                    {nullptr, nullptr}};

    return reg;
}
