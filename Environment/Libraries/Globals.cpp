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

    int decompile(lua_State *L) {
        // Since we can access original source, we will just return that
        Utilities::checkInstance(L, 1, "LuaSourceContainer");

        lua_pushvalue(L, 1);
        lua_getfield(L, -1, "Source");
        return 1;
    }

#ifdef ISNETWORKOWNER_DEV
    int isnetworkowner(lua_State *L) {
        Utilities::checkInstance(L, 1, "BasePart");
        auto part = *static_cast<void **>(lua_touserdata(L, 1));
        lua_getglobal(L, "game");
        lua_getfield(L, -1, "Players");
        lua_getfield(L, -1, "LocalPlayer");
        auto player = *static_cast<void **>(lua_touserdata(L, -1));
        lua_pop(L, 3);
        auto partSystemAddress = RBX::SystemAddress{0};
        auto localPlayerAddress = RBX::SystemAddress{0};

        uintptr_t partAddr = reinterpret_cast<uintptr_t>(part);
        std::cout << "Part address: " << reinterpret_cast<void *>(partAddr) << std::endl;

        uintptr_t part2Addr = *reinterpret_cast<uintptr_t *>(partAddr + 0x150);
        std::cout << "*(Part + 0x150): " << reinterpret_cast<void *>(part2Addr) << std::endl;

        uintptr_t part3Addr = *reinterpret_cast<uintptr_t *>(part2Addr + 0x268);
        std::cout << "*(*(Part + 0x150) + 0x268): " << reinterpret_cast<void *>(part3Addr) << std::endl;

        partSystemAddress.remoteId.peerId = part3Addr;

        std::cout << "Player address: " << player << std::endl;
        uintptr_t playerAddr = *(uintptr_t *) ((uintptr_t) player + 0x5e8);
        std::cout << "*(Player + 0x5e8): " << (void *) playerAddr << std::endl;

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
    auto *reg = new luaL_Reg[]{{"isluau", RbxStu::isluau},
                               {"httpget", RbxStu::httpget},
                               {"require", RbxStu::require},

                               {"isrbxactive", RbxStu::isrbxactive},
                               {"isgameactive", RbxStu::isrbxactive},
#ifdef ISNETWORKOWNER_DEV
                               {"isnetworkowner", RbxStu::isnetworkowner},
#endif
                               {"decompile", RbxStu::decompile},

                               {nullptr, nullptr}};
    return reg;
}
