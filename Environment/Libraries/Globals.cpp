//
// Created by roale on 18/8/2024.
//

#include "Globals.hpp"

#include "Scheduler.hpp"

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

    int getgenv(lua_State *L) {
        const auto scheduler = Scheduler::GetSingleton();
        lua_pushvalue(scheduler->GetGlobalExecutorState().value(), LUA_REGISTRYINDEX);
        lua_xmove(scheduler->GetGlobalExecutorState().value(), L, 1);
        return 1;
    }

} // namespace RbxStu


std::string Globals::GetLibraryName() const { return "rbxstu"; }
std::int32_t Globals::GetFunctionCount() const { return 0; }
luaL_Reg *Globals::GetLibraryFunctions() const {
    // WARNING: you MUST add nullptr at the end of luaL_Reg declarations, else, Luau will choke.
    auto *reg = new luaL_Reg[]{{"getrawmetatable", RbxStu::getrawmetatable},
                               {"iscclosure", RbxStu::iscclosure},
                               {"islclosure", RbxStu::islclosure},
                               {"getreg", RbxStu::getreg},
                               {"getgenv", RbxStu::getgenv},
                               {"getrenv", RbxStu::getrenv},
                               {nullptr, nullptr}};
    return reg;
}
