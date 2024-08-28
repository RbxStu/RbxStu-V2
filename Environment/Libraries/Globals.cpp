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
                               //{"require", RbxStu::require},
                               {"decompile", RbxStu::decompile},

                               {nullptr, nullptr}};
    return reg;
}
