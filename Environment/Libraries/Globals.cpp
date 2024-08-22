//
// Created by Dottik on 18/8/2024.
//

#include "Globals.hpp"

#include <HttpStatus.hpp>
#include <iostream>
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

    int fireproximityprompt(lua_State *L) {
        Utilities::checkInstance(L, 1, "ProximityPrompt");

        const auto proximityPrompt = *static_cast<std::uintptr_t **>(lua_touserdata(L, 1));
        reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ProximityPrompt_onTriggered>(
                RobloxManager::GetSingleton()->GetRobloxFunction("RBX::ProximityPrompt::onTriggered"))(proximityPrompt);
        return 0;
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
        Utilities::checkInstance(L, 1, "ModuleScript");

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

    int isrbxactive(lua_State *L) {
        lua_pushboolean(L, GetForegroundWindow() == GetCurrentProcess());
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

#ifdef ISNETWORKOWNER_DEV
    int isnetworkowner(lua_State* L) {
        Utilities::checkInstance(L, 1, "BasePart");
        auto part = *static_cast<void**>(lua_touserdata(L, 1));
        lua_getglobal(L, "game");
        lua_getfield(L, -1, "Players");
        lua_getfield(L, -1, "LocalPlayer");
        auto player = *static_cast<void**>(lua_touserdata(L, -1));
        lua_pop(L, 3);
        auto partSystemAddress = RBX::SystemAddress{0};
        auto localPlayerAddress = RBX::SystemAddress{0};

        uintptr_t partAddr = reinterpret_cast<uintptr_t>(part);
        std::cout << "Part address: " << reinterpret_cast<void*>(partAddr) << std::endl;

        uintptr_t part2Addr = *reinterpret_cast<uintptr_t*>(partAddr + 0x150);
        std::cout << "*(Part + 0x150): " << reinterpret_cast<void*>(part2Addr) << std::endl;

        uintptr_t part3Addr = *reinterpret_cast<uintptr_t*>(part2Addr + 0x268);
        std::cout << "*(*(Part + 0x150) + 0x268): " << reinterpret_cast<void*>(part3Addr) << std::endl;

        partSystemAddress.remoteId.peerId = part3Addr;

        std::cout << "Player address: " << player << std::endl;
        uintptr_t playerAddr = *(uintptr_t*)((uintptr_t)player + 0x5e8);
        std::cout << "*(Player + 0x5e8): " << (void*)playerAddr << std::endl;

        localPlayerAddress.remoteId.peerId = playerAddr;

        std::cout << "Part SystemAddress: " << partSystemAddress.remoteId.peerId << std::endl;
        std::cout << "LocalPlayer SystemAddress: " << localPlayerAddress.remoteId.peerId << std::endl;

        lua_pushnumber(L, partSystemAddress.remoteId.peerId);
        lua_pushnumber(L, localPlayerAddress.remoteId.peerId);
        lua_pushboolean(L, partSystemAddress.remoteId.peerId == localPlayerAddress.remoteId.peerId);
        return 3;
    }
#endif

} // namespace RbxStu


std::string Globals::GetLibraryName() { return "rbxstu"; }
luaL_Reg *Globals::GetLibraryFunctions() {
    // WARNING: you MUST add nullptr at the end of luaL_Reg declarations, else, Luau will choke.
    auto *reg = new luaL_Reg[]{
                               {"isluau", RbxStu::isluau},
                               {"httpget", RbxStu::httpget},
                               {"gethui", RbxStu::gethui},
                               {"fireproximityprompt", RbxStu::fireproximityprompt},
                               {"lz4compress", RbxStu::lz4compress},
                               {"lz4decompress", RbxStu::lz4decompress},
                               {"messagebox", RbxStu::messagebox},
                               {"setclipboard", RbxStu::setclipboard},
                               {"getclipboard", RbxStu::getclipboard},
                               {"emptyclipboard", RbxStu::emptyclipboard},

                               {"require", RbxStu::require},

                               {"isrbxactive", RbxStu::isrbxactive},
                               {"isgameactive", RbxStu::isrbxactive},

                               {"identifyexecutor", RbxStu::identifyexecutor},
#ifdef ISNETWORKOWNER_DEV
                               {"isnetworkowner", RbxStu::isnetworkowner},
#endif
                               {"getexecutorname", RbxStu::identifyexecutor},
                               {"decompile", RbxStu::decompile},

                               {nullptr, nullptr}};
    return reg;
}
