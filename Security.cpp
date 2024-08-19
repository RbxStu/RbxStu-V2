//
// Created by Dottik 16/8/2024.
//

#include "Security.hpp"
#include <cstdint>
#include <cstdio>
#include <lstate.h>

#include "RobloxManager.hpp"
#include "Utilities.hpp"

std::unordered_map<std::string, int> allCapabilities = {
    {"Plugin", 0x1},
    {"LocalUser", 0x2},
    {"WritePlayer", 0x4},
    {"RobloxScript", 0x8},
    {"RobloxEngine", 0x10},
    {"NotAccessible", 0x20},
    {"RunClientScript", 0x8},
    {"RunServerScript", 0x9},
    {"AccessOutsideWrite", 0xb},
    {"Unassigned", 0xf},
    {"AssetRequire", 0x10},
    {"LoadString", 0x11},
    {"ScriptGlobals", 0x12},
    {"CreateInstances", 0x13},
    {"Basic", 0x14},
    {"Audio", 0x15},
    {"DataStore", 0x16},
    {"Network", 0x17},
    {"Physics", 0x18},
    {"UI", 0x19},
    {"CSG", 0x1a},
    {"Chat", 0x1b},
    {"Animation", 0x1c},
    {"Avatar", 0x1d},
    {"Assistant", 0x3e}
};

std::unordered_map<int, std::list<std::string>> identityCapabilities = {
    {3, {"RunServerScript", "Plugin", "LocalUser", "RobloxScript", "RunClientScript", "AccessOutsideWrite"}},
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
        if ((capability->second & capabilities) == capability->second) {
            logger->PrintInformation(RbxStu::Security, capability->first);
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
                capabilities |= it->second;
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

bool Security::SetLuaClosureSecurity(Closure *lClosure) {
    if (lClosure->isC)
        return false;
    const auto pProto = lClosure->l.p;
    auto *pMem = pProto->userdata != nullptr ? static_cast<std::uintptr_t *>(pProto->userdata)
                                             : static_cast<std::uintptr_t *>(malloc(sizeof(std::uintptr_t)));

    *pMem = 0x3FBF897F |
            0b100000000000000000000000000000000; // 0x3FFFF00 |
                                                 // RobloxManager::GetSingleton()->IdentityToCapability(8).value();
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
