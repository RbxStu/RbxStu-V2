//
// Created by Pixeluted on 22/08/2024.
//

#include "Instance.hpp"

#include <iostream>

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

        int isnetworkowner(lua_State *L) {
            Utilities::checkInstance(L, 1, "BasePart");
            auto part = *static_cast<void **>(lua_touserdata(L, 1));

            /*
             *  Quick rundown due to the hack around this function.
             *  We would normally access remoteId/peerId, but due to the fact that we are WE, we don't have that
             *  privilage, instead we must get the peerId of a part we ACTUALLY own, this wouldn't be that complicated.
             *
             *  But what is to be noted is the following:
             *      - On Local clients, RemoteId/PeerId is unreachable, PeerId == -1    (uint32_t max [underflowing]).
             *      - Anchored/Bound by a physics constraint/welded, PeerId == 2        (After trial and error)
             *
             *  Thus, the hack around this behaviour is to create a part ourselves. Parts created by ourselves that are
             *  parented to workspace are automatically simulated by our player, this means we really don't need
             *  anything else, and we can obtain our remote PeerId that way. This COULD be hardcoded, but then team
             *  tests would not work as expected. (PeerId appears to start at 4). We _cannot_ rely on HumanoidRootPart,
             *  as it can be owned by the server if the game developer desiers it.
             */

            auto partSystemAddress = RBX::SystemAddress{0};
            auto localPlayerAddress = RBX::SystemAddress{0};
            reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_BasePart_getNetworkOwner>(
                    RobloxManager::GetSingleton()->GetRobloxFunction("RBX::BasePart::getNetworkOwner"))(
                    part, &partSystemAddress);

            lua_getglobal(L, "Instance");
            lua_getfield(L, -1, "new");
            lua_pushstring(L, "Part");
            lua_getglobal(L, "workspace");
            lua_pcall(L, 2, 1, 0);
            reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_BasePart_getNetworkOwner>(
                    RobloxManager::GetSingleton()->GetRobloxFunction("RBX::BasePart::getNetworkOwner"))(
                    *static_cast<void **>(lua_touserdata(L, -1)), &localPlayerAddress);

            lua_getfield(L, -1, "Destroy");
            lua_pushvalue(L, -2);
            lua_pcall(L, 1, 0, 0);
            lua_pop(L, 1);

            lua_pushboolean(L, partSystemAddress.remoteId.peerId == 2 ||
                                       partSystemAddress.remoteId.peerId == localPlayerAddress.remoteId.peerId);
            return 1;
        }

        int isscriptable(lua_State *L) {
            Utilities::checkInstance(L, 1, "ANY");
            const auto propName = std::string(luaL_checkstring(L, 2));

            for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
                 const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
                if (prop->name == propName) {
                    lua_pushboolean(L, prop->IsScriptable());
                }
            }
            if (lua_type(L, -1) != lua_Type::LUA_TBOOLEAN)
                lua_pushnil(L);

            return 1;
        }

        int setscriptable(lua_State *L) {
            Utilities::checkInstance(L, 1, "ANY");
            const auto propName = std::string(luaL_checkstring(L, 2));
            const bool newScriptable = luaL_checkboolean(L, 3);

            for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
                 const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
                if (prop->name == propName) {
                    lua_pushboolean(L, prop->IsScriptable());
                    prop->SetScriptable(newScriptable);
                }
            }
            if (lua_gettop(L) == 3)
                luaL_argerror(L, 2,
                              std::format("userdata<{}> does not have the property '{}'.",
                                          Utilities::getInstanceType(L, 1), propName)
                                      .c_str());

            return 1;
        }

        int gethiddenproperty(lua_State *L) {
            Utilities::checkInstance(L, 1, "ANY");
            const auto propName = std::string(luaL_checkstring(L, 2));

            bool isPublic = false;
            for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
                 const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
                if (prop->name == propName) {
                    isPublic = prop->IsPublic();
                    const auto isScriptable = prop->IsScriptable();
                    prop->SetIsPublic(true);
                    prop->SetScriptable(true);
                    lua_getfield(L, 1, propName.c_str());
                    prop->SetScriptable(isScriptable);
                    prop->SetIsPublic(isPublic);
                }
            }
            if (lua_gettop(L) == 2)
                luaL_argerror(L, 2,
                              std::format("userdata<{}> does not have the property '{}'.",
                                          Utilities::getInstanceType(L, 1), propName)
                                      .c_str());

            lua_pushboolean(L, !isPublic);
            return 2;
        }

        int sethiddenproperty(lua_State *L) {
            Utilities::checkInstance(L, 1, "ANY");
            const auto propName = std::string(luaL_checkstring(L, 2));
            luaL_checkany(L, 3);

            bool isPublic = false;
            for (const auto instance = *static_cast<RBX::Instance **>(lua_touserdata(L, 1));
                 const auto &prop: instance->classDescriptor->propertyDescriptors.descriptors) {
                if (prop->name == propName) {
                    isPublic = prop->IsPublic();
                    const auto isScriptable = prop->IsScriptable();
                    prop->SetIsPublic(true);
                    prop->SetScriptable(true);
                    lua_setfield(L, 1, propName.c_str());
                    prop->SetScriptable(isScriptable);
                    prop->SetIsPublic(isPublic);
                }
            }
            if (lua_gettop(L) == 3) // lua_setfield will pop the new value of the property.
                luaL_argerror(L, 2,
                              std::format("userdata<{}> does not have the property '{}'.",
                                          Utilities::getInstanceType(L, 1), propName)
                                      .c_str());

            lua_pushboolean(L, !isPublic);
            return 1;
        }
    } // namespace Instance
} // namespace RbxStu

std::string Instance::GetLibraryName() { return "instances"; }
luaL_Reg *Instance::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"gethui", RbxStu::Instance::gethui},
                                    {"fireproximityprompt", RbxStu::Instance::fireproximityprompt},

                                    {"isnetworkowner", RbxStu::Instance::isnetworkowner},

                                    {"isscriptable", RbxStu::Instance::isscriptable},
                                    {"setscriptable", RbxStu::Instance::setscriptable},
                                    {"gethiddenproperty", RbxStu::Instance::gethiddenproperty},
                                    {"sethiddenproperty", RbxStu::Instance::sethiddenproperty},
                                    {nullptr, nullptr}};

    return reg;
}
