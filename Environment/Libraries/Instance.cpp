//
// Created by Pixeluted on 22/08/2024.
//

#include "Instance.hpp"

#include "RobloxManager.hpp"
#include "Utilities.hpp"

namespace RbxStu {
    namespace Instance {
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

        int fireproximityprompt(lua_State *L) {
            Utilities::checkInstance(L, 1, "ProximityPrompt");

            const auto proximityPrompt = *static_cast<std::uintptr_t **>(lua_touserdata(L, 1));
            reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ProximityPrompt_onTriggered>(
                    RobloxManager::GetSingleton()->GetRobloxFunction("RBX::ProximityPrompt::onTriggered"))(
                    proximityPrompt);
            return 0;
        }
    } // namespace Instance
} // namespace RbxStu

std::string Instance::GetLibraryName() { return "instances"; }
luaL_Reg *Instance::GetLibraryFunctions() {
    auto reg = new luaL_Reg[]{{"gethui", RbxStu::Instance::gethui},
                              {"fireproximityprompt", RbxStu::Instance::fireproximityprompt},

                              {nullptr, nullptr}};

    return reg;
}
