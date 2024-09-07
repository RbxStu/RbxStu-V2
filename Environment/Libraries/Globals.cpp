//
// Created by Dottik on 18/8/2024.
//

#include "Globals.hpp"

#include <HttpStatus.hpp>
#include <iostream>
#include <lz4.h>

#include "ClosureManager.hpp"
#include "Communication/Communication.hpp"
#include "Luau/CodeGen/include/Luau/CodeGen.h"
#include "Luau/Compiler.h"
#include "RobloxManager.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "cpr/api.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lstring.h"

namespace RbxStu {
    int isluau(lua_State *L) {
        lua_pushboolean(L, true);
        return 1;
    }

    int makeuncollectable(lua_State *L) {
        luaL_checkany(L, 1);
        luaC_threadbarrier(L);
        s_mRefsMap[luaA_toobject(L, 1)] = lua_ref(L, 1);
        return 0;
    }

    int makecollectable(lua_State *L) {
        luaL_checkany(L, 1);
        const auto *const obj = luaA_toobject(L, 1);

        if (obj->tt == lua_Type::LUA_TFUNCTION && obj->value.gc->cl.isC) { // This could have dire consequences...
            luaL_error(L, "C closures cannot be made collectable");
        } else if (obj->tt == lua_Type::LUA_TFUNCTION &&
                   !obj->value.gc->cl.isC) { // This could have dire consequences...
            // We must check if this closure is part of a Wrapped C Closure, if so, we cannot make it collectable, else
            // on call, the newcclosure will crash us.
            if (ClosureManager::GetSingleton()->IsWrapped(static_cast<const Closure *>(obj->value.p)))
                luaL_error(L, "luau closures wrapped with a new C closure cannot be made collectable");
        } else if (obj->value.p == L->global->registry.value.p) {
            luaL_error(L, "The luau registry cannot be made collectable");
        }

        luaC_threadbarrier(L);

        // If an object was referenced in the Lua Registry, we must find it, and unref it to allow it to be collected
        // Which steps aside from making it white (refer to luaC_init).
        if (s_mRefsMap.contains(luaA_toobject(L, 1))) {
            lua_unref(L, s_mRefsMap.at(luaA_toobject(L, 1)));
            s_mRefsMap.erase(luaA_toobject(L, 1));
        }

        obj->value.gc->gch.marked = luaC_white(L->global);
        return 0;
    }

} // namespace RbxStu


std::string Globals::GetLibraryName() { return "rbxstu"; }
luaL_Reg *Globals::GetLibraryFunctions() {
    // WARNING: you MUST add nullptr at the end of luaL_Reg declarations, else, Luau will choke.
    const auto reg = new luaL_Reg[]{{"isluau", RbxStu::isluau},
                                    // {"decompile", RbxStu::decompile}, // Stripped for the reasons of security.
                                    {"makeuncollectable", RbxStu::makeuncollectable},
                                    {"makecollectable", RbxStu::makecollectable},
                                    {nullptr, nullptr}};

    return reg;
}
