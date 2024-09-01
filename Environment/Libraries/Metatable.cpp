//
// Created by Pixeluted on 22/08/2024.
//

#include "Metatable.hpp"

namespace RbxStu {
    namespace Metatable {
        int getrawmetatable(lua_State *L) {
            luaL_checkany(L, 1);

            if (!lua_getmetatable(L, 1))
                lua_pushnil(L);

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

        int setnamecallmethod(lua_State *L) {
            luaL_checkstring(L, 1);
            if (L->namecall != nullptr)
                L->namecall = &L->top->value.gc->ts;

            return 0;
        }

        int setrawmetatable(lua_State *L) {
            luaL_argexpected(L,
                             lua_istable(L, 1) || lua_islightuserdata(L, 1) || lua_isuserdata(L, 1) ||
                                     lua_isbuffer(L, 1) || lua_isvector(L, 1),
                             1, "table or userdata or vector or buffer");

            luaL_argexpected(L, lua_istable(L, 2) || lua_isnil(L, 2), 2, "table or nil");

            lua_setmetatable(L, 1);
            return 0;
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

        int hookmetamethod(lua_State *L) {
            luaL_checkany(L, 1);
            auto mtName = luaL_checkstring(L, 2);
            luaL_checktype(L, 3, lua_Type::LUA_TFUNCTION);

            lua_pushvalue(L, 1);
            lua_getmetatable(L, -1);
            if (lua_getfield(L, -1, mtName) == LUA_TNIL) {
                luaL_argerrorL(
                        L, 2,
                        std::format("'{}' is not a valid member of the given object's metatable.", mtName).c_str());
            }
            lua_setreadonly(L, -2, false);

            lua_getglobal(L, "hookfunction");
            lua_pushvalue(L, -2);
            lua_pushvalue(L, 3);
            lua_call(L, 2, 1);
            lua_remove(L, -2);
            lua_setreadonly(L, -2, true);

            return 1;
        }
    } // namespace Metatable
} // namespace RbxStu

std::string Metatable::GetLibraryName() { return "metatable"; }
luaL_Reg *Metatable::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"getrawmetatable", RbxStu::Metatable::getrawmetatable},
                                     {"setrawmetatable", RbxStu::Metatable::setrawmetatable},

                                     {"hookmetamethod", RbxStu::Metatable::hookmetamethod},
                                     {"getnamecallmethod", RbxStu::Metatable::getnamecallmethod},
                                     {"setnamecallmethod", RbxStu::Metatable::setnamecallmethod},

                                     {"isreadonly", RbxStu::Metatable::isreadonly},
                                     {"setreadonly", RbxStu::Metatable::setreadonly},

                                     {nullptr, nullptr}};

    return reg;
}
