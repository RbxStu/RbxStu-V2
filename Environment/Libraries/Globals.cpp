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
    auto *reg = new luaL_Reg[]{{"isluau", RbxStu::isluau},
                               {"httpget", RbxStu::httpget},
                               {"require", RbxStu::require},

                               {"decompile", RbxStu::decompile},

                               {nullptr, nullptr}};
    return reg;
}
