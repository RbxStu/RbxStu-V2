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
    } // namespace Instance
} // namespace RbxStu

std::string Instance::GetLibraryName() { return "instances"; }
luaL_Reg *Instance::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"gethui", RbxStu::Instance::gethui},
                              {"fireproximityprompt", RbxStu::Instance::fireproximityprompt},

#ifdef ISNETWORKOWNER_DEV
                              {"isnetworkowner", RbxStu::Instance::isnetworkowner},
#endif

                              {nullptr, nullptr}};

    return reg;
}
