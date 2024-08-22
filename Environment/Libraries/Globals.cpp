//
// Created by Dottik on 18/8/2024.
//

#include "Globals.hpp"

#include <HttpStatus.hpp>
#include <lz4.h>

#include "ClosureManager.hpp"
#include "Communication.hpp"
#include "Luau/CodeGen/include/Luau/CodeGen.h"
#include "Luau/Compiler.h"
#include "RobloxManager.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "cpr/api.h"
#include "lfunc.h"
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
        // Utilities::RobloxThreadSuspension threadSuspension(true);
        // auto rL = Scheduler::GetSingleton()->GetGlobalRobloxState().value();
        // lua_pushvalue(rL, LUA_GLOBALSINDEX);
        // lua_xmove(rL, L, 1);
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        return 1;
    }

    int getgenv(lua_State *L) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
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

    int gethui(lua_State *L) {
        // Equivalent to cloneref(cloneref(cloneref(game):GetService("CoreGui")).RobloxGui)
        // Excessive clonereffing, I made the cloneref, i will use all of it!
        // - Dottik
        lua_getglobal(L, "cloneref");
        lua_getglobal(L, "game");
        lua_call(L, 1, 1);
        lua_getfield(L, -1, "GetService");
        lua_pushvalue(L, -2);
        lua_pushstring(L, "CoreGui");
        lua_call(L, 2, 1);
        lua_remove(L, 1); // CoreGui is alone on the stack.
        lua_getglobal(L, "cloneref");
        lua_pushvalue(L, 1);
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
            luaL_argerror(L, 1, "Invalid protocol (expected 'http://' or 'https://')");

        const auto scheduler = Scheduler::GetSingleton();

        scheduler->ScheduleJob(SchedulerJob(
                L, [url](lua_State *L, std::shared_future<std::function<int(lua_State *)>> *callbackToExecute) {
                    const auto response = cpr::Get(cpr::Url{url}, cpr::Header{{"User-Agent", "Roblox/WinInet"}});

                    auto output = std::string("");

                    if (HttpStatus::IsError(response.status_code)) {
                        output = std::format(
                                "HttpGet failed\nResponse {} - {}. {}", std::to_string(response.status_code),
                                HttpStatus::ReasonPhrase(response.status_code), std::string(response.error.message));
                    } else {
                        output = response.text;
                    }

                    *callbackToExecute = std::async(std::launch::async, [output]() -> std::function<int(lua_State *)> {
                        return [output](lua_State *L) -> int {
                            lua_pushlstring(L, output.c_str(), output.size());
                            return 1;
                        };
                    });
                }));

        L->ci->flags |= 1;
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

        luaL_argexpected(L, lua_istable(L, 2) || lua_isnil(L, 2), 2, "table or nil");

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
        Utilities::checkInstance(L, 1, "ProximityPrompt");

        const auto proximityPrompt = *static_cast<std::uintptr_t **>(lua_touserdata(L, 1));
        reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ProximityPrompt_onTriggered>(
                RobloxManager::GetSingleton()->GetRobloxFunction("RBX::ProximityPrompt::onTriggered"))(proximityPrompt);
        return 0;
    }

    int cloneref(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");

        const auto userdata = lua_touserdata(L, -1);
        const auto rawUserdata = *static_cast<void **>(userdata);
        const auto robloxManager = RobloxManager::GetSingleton();
        lua_pushlightuserdata(L, robloxManager->GetRobloxFunction("RBX::Instance::pushInstance"));
        lua_rawget(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, rawUserdata);
        lua_rawget(L, -2);

        lua_pushlightuserdata(L, rawUserdata);
        lua_pushnil(L);
        lua_rawset(L, -4);

        reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_Instance_pushInstance>(
                robloxManager->GetRobloxFunction("RBX::Instance::pushInstance"))(L, userdata);
        lua_pushlightuserdata(L, rawUserdata);
        lua_pushvalue(L, -3);
        lua_rawset(L, -5);
        return 1;
    }

    int loadstring(lua_State *L) {
        const auto luauCode = luaL_checkstring(L, 1);
        const auto chunkName = luaL_optstring(L, 2, "RbxStuV2_LoadString");
        constexpr auto compileOpts = Luau::CompileOptions{1, 2};
        const auto bytecode = Luau::compile(luauCode, compileOpts);

        if (luau_load(L, chunkName, bytecode.c_str(), bytecode.size(), 0) != lua_Status::LUA_OK) {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        if (Communication::GetSingleton()->IsCodeGenerationEnabled()) {
            const Luau::CodeGen::CompilationOptions opts{0};
            Logger::GetSingleton()->PrintInformation(
                    RbxStu::Anonymous, "Native Code Generation is enabled! Compiling Luau Bytecode -> Native");
            Luau::CodeGen::compile(L, -1, opts);
        }


        Security::GetSingleton()->SetLuaClosureSecurity(lua_toclosure(L, -1), 8);
        lua_setsafeenv(L, LUA_GLOBALSINDEX, false); // env is not safe anymore.
        return 1;
    }

    int lz4compress(lua_State *L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        const char *data = lua_tostring(L, 1);
        int iMaxCompressedSize = LZ4_compressBound(strlen(data));
        const auto pszCompressedBuffer = new char[iMaxCompressedSize];
        memset(pszCompressedBuffer, 0, iMaxCompressedSize);

        LZ4_compress(data, pszCompressedBuffer, strlen(data));
        lua_pushlstring(L, pszCompressedBuffer, iMaxCompressedSize);
        return 1;
    }

    int lz4decompress(lua_State *L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TNUMBER);

        const char *data = lua_tostring(L, 1);
        const int data_size = lua_tointeger(L, 2);

        auto *pszUncompressedBuffer = new char[data_size];

        memset(pszUncompressedBuffer, 0, data_size);

        LZ4_uncompress(data, pszUncompressedBuffer, data_size);
        lua_pushlstring(L, pszUncompressedBuffer, data_size);
        return 1;
    }

    int messagebox(lua_State *L) {
        const auto text = luaL_checkstring(L, 1);
        const auto caption = luaL_checkstring(L, 2);
        const auto type = luaL_checkinteger(L, 3);
        Scheduler::GetSingleton()->ScheduleJob(SchedulerJob(
                L, [text, caption, type](lua_State *L, std::shared_future<std::function<int(lua_State *)>> *future) {
                    const int lMessageboxReturn = MessageBoxA(nullptr, text, caption, type);

                    *future = std::async(std::launch::async, [lMessageboxReturn]() -> std::function<int(lua_State *)> {
                        return [lMessageboxReturn](lua_State *L) -> int {
                            lua_pushinteger(L, lMessageboxReturn);
                            return 1;
                        };
                    });
                }));

        L->ci->flags |= 1;
        return lua_yield(L, 1);
    }

    int require(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TUSERDATA);

        auto moduleScript = *static_cast<RBX::ModuleScript **>(lua_touserdata(L, 1));
        moduleScript->m_bIsRobloxScriptModule = true;
        auto robloxState = Scheduler::GetSingleton()->GetGlobalRobloxState().value();
        lua_getglobal(robloxState, "require");
        lua_xmove(robloxState, L, 1);
        lua_pushvalue(L, 1);
        auto status = lua_pcall(L, 1, 1, 0);
        moduleScript->m_bIsRobloxScriptModule = false;

        if (status != lua_Status::LUA_OK)
            lua_error(L);

        return 1;
    }

    int setidentity(lua_State *L) {
        luaL_checkinteger(L, 1);
        auto newIdentity = lua_tointeger(L, 1);
        const auto security = Security::GetSingleton();
        security->SetThreadSecurity(L, newIdentity);

        // If calling closure is LClosure and is not base CI, set its capabilities too
        if (L->base_ci != L->ci) {
            auto callingClosure = (L->ci - 1)->func->value.gc->cl;
            if (callingClosure.isC == 0) {
                security->SetLuaClosureSecurity(&callingClosure, newIdentity);
            }
        }

        // Capabilities and identity are applied next resumption cycle, we need to yield!
        const auto scheduler = Scheduler::GetSingleton();
        scheduler->ScheduleJob(SchedulerJob(
                L, [](lua_State *L, std::shared_future<std::function<int(lua_State *)>> *callbackToExecute) {
                    *callbackToExecute = std::async(std::launch::async, [L]() -> std::function<int(lua_State *)> {
                        Sleep(1);
                        return [](lua_State *L) { return 0; };
                    });
                }));


        return lua_yield(L, 0);
    }

    int getidentity(lua_State *L) {
        auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
        lua_pushinteger(L, plStateUd->identity);

        return 1;
    }

    int printcaps(lua_State *L) {
        auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
        const auto security = Security::GetSingleton();
        security->PrintCapabilities(plStateUd->capabilities);

        return 0;
    }

    int isrbxactive(lua_State *L) {
        lua_pushboolean(L, GetForegroundWindow() == GetCurrentProcess());
        return 1;
    }


    int isourclosure(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TFUNCTION);

        if (const auto pClosure = lua_toclosure(L, 1); pClosure->isC) {
            lua_pushboolean(L, pClosure->c.debugname == nullptr ||
                                       ClosureManager::GetSingleton()->IsWrappedCClosure(pClosure));
        } else {
            lua_pushboolean(L, pClosure->l.p->linedefined == -1);
        }

        return 1;
    }

    int setclipboard(lua_State *L) {
        luaL_checkstring(L, 1);
        const char *text = lua_tostring(L, 1);

        if (!OpenClipboard(nullptr))
            luaL_error(L, "Failed to open clipboard");


        if (!EmptyClipboard()) {
            CloseClipboard();
            luaL_error(L, "Failed to empty clipboard");
        }

        const size_t textLength = strlen(text) + 1;
        const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, textLength);
        if (!hMem) {
            CloseClipboard();
            luaL_error(L, "Failed to allocate memory in global space");
        }

        const auto pMem = static_cast<char *>(GlobalLock(hMem));
        if (!pMem) {
            GlobalFree(hMem);
            CloseClipboard();
            luaL_error(L, "Failed to lock global memory");
        }

        memcpy(pMem, text, textLength);

        if (GlobalUnlock(hMem) != 0 && GetLastError() != NO_ERROR) {
            GlobalFree(hMem);
            CloseClipboard();
            luaL_error(L, "Failed to unlock global memory");
        }

        if (!SetClipboardData(CF_TEXT, hMem)) {
            GlobalFree(hMem);
            CloseClipboard();
            luaL_error(L, "Failed to set clipboard data");
        }

        if (!CloseClipboard())
            luaL_error(L, "Failed to close clipboard");

        return 0;
    }

    int getclipboard(lua_State *L) {
        if (!Communication::GetSingleton()->IsUnsafeMode()) {
            Logger::GetSingleton()->PrintInformation(
                    RbxStu::Anonymous, "getclipboard call has returned a stub value, as getclipboard is considered an "
                                       "unsafe function, and will only be available when Unsafe mode is enabled.");
            lua_pushstring(L, "");
            return 1;
        }

        if (!OpenClipboard(nullptr))
            luaL_error(L, "Failed to open clipboard");

        HANDLE hData = GetClipboardData(CF_TEXT);
        if (!hData) {
            CloseClipboard();
            luaL_error(L, "Failed to get clipboard data");
        }

        const char *clipboardText = static_cast<const char *>(GlobalLock(hData));
        if (!clipboardText) {
            CloseClipboard();
            luaL_error(L, "Failed to lock clipboard data");
        }

        lua_pushstring(L, clipboardText);

        if (GlobalUnlock(hData) == 0 && GetLastError() != NO_ERROR) {
            CloseClipboard();
            luaL_error(L, "Failed to unlock clipboard data");
        }

        if (!CloseClipboard())
            luaL_error(L, "Failed to close clipboard");

        return 1;
    }

    int emptyclipboard(lua_State *L) {
        if (!OpenClipboard(nullptr))
            luaL_error(L, "Failed to open clipboard");


        if (!EmptyClipboard()) {
            CloseClipboard();
            luaL_error(L, "Failed to empty clipboard");
        }


        if (!CloseClipboard())
            luaL_error(L, "Failed to close clipboard");

        return 0;
    }

    int identifyexecutor(lua_State *L) {
        lua_pushstring(L, "RbxStu");
        lua_pushstring(L, "V2");
        return 2;
    }

    int decompile(lua_State *L) {
        // Since we can access original source, we will just return that
        Utilities::checkInstance(L, 1, "LuaSourceContainer");

        lua_pushvalue(L, 1);
        lua_getfield(L, -1, "Source");
        return 1;
    }
} // namespace RbxStu


std::string Globals::GetLibraryName() { return "rbxstu"; }
luaL_Reg *Globals::GetLibraryFunctions() {
    // WARNING: you MUST add nullptr at the end of luaL_Reg declarations, else, Luau will choke.
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
                               {"loadstring", RbxStu::loadstring},
                               {"lz4compress", RbxStu::lz4compress},
                               {"lz4decompress", RbxStu::lz4decompress},
                               {"messagebox", RbxStu::messagebox},
                               {"setidentity", RbxStu::setidentity},
                               {"setthreadcontext", RbxStu::setidentity},
                               {"setthreadidentity", RbxStu::setidentity},
                               {"setclipboard", RbxStu::setclipboard},
                               {"getclipboard", RbxStu::getclipboard},
                               {"emptyclipboard", RbxStu::emptyclipboard},

                               {"getidentity", RbxStu::getidentity},
                               {"getthreadidentity", RbxStu::getidentity},
                               {"getthreadcontext", RbxStu::getidentity},

                               {"printcaps", RbxStu::printcaps},
                               {"require", RbxStu::require},

                               {"isrbxactive", RbxStu::isrbxactive},
                               {"isgameactive", RbxStu::isrbxactive},

                               {"isourclosure", RbxStu::isourclosure},
                               {"checkclosure", RbxStu::isourclosure},
                               {"isexecutorclosure", RbxStu::isourclosure},
                               {"identifyexecutor", RbxStu::identifyexecutor},

                               {"getexecutorname", RbxStu::identifyexecutor},
                               {"decompile", RbxStu::decompile},

                               {"clonefunction", ClosureManager::clonefunction},
                               {"hookfunction", ClosureManager::hookfunction},
                               {"replaceclosure", ClosureManager::hookfunction},

                               {"unhookfunction", ClosureManager::unhookfunction},
                               {"restorefunction", ClosureManager::unhookfunction},

                               {"newlclosure", ClosureManager::newlclosure},
                               {"newcclosure", ClosureManager::newcclosure},


                               {nullptr, nullptr}};
    return reg;
}
