//
// Created by Dottik 16/8/2024.
//

#include "Security.hpp"
#include <cstdint>
#include <cstdio>
#include <lstate.h>

#include "RobloxManager.hpp"
#include "Utilities.hpp"

// Capability Name, Capability, IsUsingBITTESTQ?
std::unordered_map<std::string, std::pair<int, bool>> allCapabilities = {
    {"Plugin", {0x1, false}},
    {"LocalUser", {0x2, false}},
    {"WritePlayer", {0x4, false}},
    {"RobloxScript", {0x8, false}},
    {"RobloxEngine", {0x10, false}},
    {"NotAccessible", {0x20, false}},
    {"RunClientScript", {0x8, true}},
    {"RunServerScript", {0x9, true}},
    {"AccessOutsideWrite", {0xb, true}},
    {"Unassigned", {0xf, true}},
    {"AssetRequire", {0x10, true}},
    {"LoadString", {0x11, true}},
    {"ScriptGlobals", {0x12, true}},
    {"CreateInstances", {0x13, true}},
    {"Basic", {0x14, true}},
    {"Audio", {0x15, true}},
    {"DataStore", {0x16, true}},
    {"Network", {0x17, true}},
    {"Physics", {0x18, true}},
    {"UI", {0x19, true}},
    {"CSG", {0x1a, true}},
    {"Chat", {0x1b, true}},
    {"Animation", {0x1c, true}},
    {"Avatar", {0x1d, true}},
    {"Assistant", {0x3e, true}}
};

std::unordered_map<int, std::list<std::string>> identityCapabilities = {
    {3, {"RunServerScript", "Plugin", "LocalUser", "RobloxScript", "RunClientScript", "AccessOutsideWrite"}},
    {2, {"CSG", "Chat", "Animation", "Avatar"}}, // These are needed for 'require' to work!
    {4, {"Plugin", "LocalUser"}},
    {6, {"RunServerScript", "Plugin", "LocalUser", "RobloxScript", "RunClientScript", "AccessOutsideWrite"}},
    {8, {"ScriptGlobals", "RunServerScript", "Plugin", "Chat", "CreateInstances", "LocalUser", "RobloxEngine", "WritePlayer", "RobloxScript", "CSG", "NotAccessible", "RunClientScript", "AccessOutsideWrite", "Physics", "Unassigned", "AssetRequire", "Avatar", "LoadString", "Basic", "Audio", "DataStore", "Network", "UI", "Animation", "Assistant"}}
};

std::shared_ptr<Security> Security::pInstance;

std::shared_ptr<Security> Security::GetSingleton() {
    if (Security::pInstance == nullptr)
        Security::pInstance = std::make_shared<Security>();

    return Security::pInstance;
}

void Security::PrintCapabilities(int capabilities) {
    const auto logger = Logger::GetSingleton();

    logger->PrintInformation(RbxStu::Security, std::format("0x{:X} got these capabilities:", capabilities));
    for (auto capability = allCapabilities.begin(); capability != allCapabilities.end(); ++capability) {
        if (capability->second.second == true) {
            if ((capabilities) & (1 << capability->second.first)) {
                logger->PrintInformation(RbxStu::Security, capability->first);
            }
        } else {
            if ((capability->second.first & capabilities) == capability->second.first) {
                logger->PrintInformation(RbxStu::Security, capability->first);
            }
        }
    }
};

int Security::IdentityToCapabilities(int identity) {
    int capabilities = 0x3FFFF00 | 0b100000000; // Basic capability | Checkcaller check
    auto capabilitiesForIdentity = identityCapabilities.find(identity);

    if (capabilitiesForIdentity != identityCapabilities.end()) {
        for (const auto& capability : capabilitiesForIdentity->second) {
            auto it = allCapabilities.find(capability);
            if (it != allCapabilities.end()) {
                if (it->second.second == true) {
                    capabilities |= (1 << it->second.first);
                } else {
                    capabilities |= it->second.first;
                }
            } else {
                Logger::GetSingleton()->PrintWarning(RbxStu::Security, std::format("Couldn't find capability {} in allCapabilities", capability));
            }
        }
    }

    return capabilities;
}

void Security::SetThreadSecurity(lua_State *L) {
    if (!Utilities::IsPointerValid(static_cast<RBX::Lua::ExtraSpace *>(L->userdata)))
        L->global->cb.userthread(L->global->mainthread,
                                 L); // If unallocated, then we must run the callback to create a valid RobloxExtraSpace

    auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);

    const auto security = Security::GetSingleton();
    auto capabilities = security->IdentityToCapabilities(8);

    const auto logger = Logger::GetSingleton();
    logger->PrintInformation(RbxStu::Security, std::format("Elevating our thread capabilities to: 0x{:X}", capabilities));

    plStateUd->identity = 8;
    plStateUd->capabilities = capabilities;
}

static void set_proto(Proto *proto, uintptr_t *proto_identity) {
    // NEVER FORGET TO SET THE PROTOS and SUB PROTOS USERDATA!!
    proto->userdata = static_cast<void *>(proto_identity);
    for (auto i = 0; i < proto->sizep; i++)
        set_proto(proto->p[i], proto_identity);
}

bool Security::IsOurThread(lua_State *L) {
    /// The way we currently have of checking if a thread is our thread is through... capabilities!
    /// Roblox handles capabilities using an std::int64_t, giving us 64 fun bits to play around with.
    /// This way, we can set the bit 9th, used to describe if the thread has RunScriptServer capabilities, to set it as
    /// our thread. Then we & it to validate it is present on the integer with an AND, which it shouldn't be ever if its
    /// anything normal, but we aren't normal!
    const auto extraSpace = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
    const auto logger = Logger::GetSingleton();
    logger->PrintInformation(RbxStu::Security, std::format("Thread Capabilities: {}", extraSpace->capabilities));
    const auto passed = (extraSpace->capabilities & 0b100000000) == 0b100000000;
    logger->PrintInformation(RbxStu::Security, std::format("isourthread: {}", passed ? 1 : 0));
    return passed;
}

bool Security::SetLuaClosureSecurity(Closure *lClosure, int identity) {
    if (lClosure->isC)
        return false;
    const auto pProto = lClosure->l.p;
    auto *pMem = pProto->userdata != nullptr ? static_cast<std::uintptr_t *>(pProto->userdata)
                                             : static_cast<std::uintptr_t *>(malloc(sizeof(std::uintptr_t)));
    
    *pMem = Security::GetSingleton()->IdentityToCapabilities(identity);
    set_proto(pProto, pMem);
    return true;
}

void Security::WipeClosurePrototype(Closure *lClosure) {
    if (lClosure->isC)
        return;
    auto proto = lClosure->l.p;
    proto->debugname = nullptr;
    proto->linedefined = -1;
}
