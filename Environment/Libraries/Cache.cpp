//
// Created by Pixeluted on 22/08/2024.
//

#include "Cache.hpp"

#include "RobloxManager.hpp"

namespace RbxStu {
    namespace Cache {
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

        int compareinstances(lua_State *L) {
            Utilities::checkInstance(L, 1, "ANY");
            Utilities::checkInstance(L, 2, "ANY");

            lua_pushboolean(L, *static_cast<const std::uintptr_t *>(lua_touserdata(L, 1)) ==
                                       *static_cast<const std::uintptr_t *>(lua_touserdata(L, 2)));

            return 1;
        }

        // TODO: Make cache.replace, cache.iscached, cache.invalidate (Name them here just "replace", "iscached",
        // "invalidate")
    } // namespace Cache
} // namespace RbxStu

std::string Cache::GetLibraryName() { return "cache"; };
luaL_Reg *Cache::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"cloneref", RbxStu::Cache::cloneref},
                                    {"invalidate", RbxStu::Cache::cloneref},

                                    {"compareinstances", RbxStu::Cache::compareinstances},

                                    {nullptr, nullptr}};

    return reg;
}
