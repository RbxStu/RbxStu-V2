//
// Created by Dottik 16/8/2024.
//

#include "Security.hpp"
#include <cstdint>
#include <cstdio>
#include <lstate.h>

#include "RobloxManager.hpp"
#include "Utilities.hpp"
#include "lstring.h"

// Capability Name, Capability, IsUsingBITTESTQ?
std::unordered_map<std::string, std::pair<std::uint64_t, bool>> allCapabilities = {{"Plugin", {0x1, false}},
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
                                                                                   {"Assistant", {0x3e, true}}};

std::unordered_map<std::int32_t, std::list<std::string>> identityCapabilities = {
        {3,
         {"RunServerScript", "Plugin", "LocalUser", "RobloxScript", "RunClientScript", "AccessOutsideWrite", "Avatar"}},
        {2, {"CSG", "Chat", "Animation", "Avatar"}}, // These are needed for 'require' to work!
        {4, {"Plugin", "LocalUser", "Avatar"}},
        {6,
         {"RunServerScript", "Plugin", "LocalUser", "Avatar", "RobloxScript", "RunClientScript", "AccessOutsideWrite"}},
        {8,
         {"ScriptGlobals",
          "RunServerScript",
          "Plugin",
          "Chat",
          "CreateInstances",
          "LocalUser",
          "RobloxEngine",
          "WritePlayer",
          "RobloxScript",
          "CSG",
          "NotAccessible",
          "RunClientScript",
          "AccessOutsideWrite",
          "Physics",
          "Unassigned",
          "AssetRequire",
          "Avatar",
          "LoadString",
          "Basic",
          "Audio",
          "DataStore",
          "Network",
          "UI",
          "Animation",
          "Assistant"}}};

std::shared_ptr<Security> Security::pInstance;

std::shared_ptr<Security> Security::GetSingleton() {
    if (Security::pInstance == nullptr)
        Security::pInstance = std::make_shared<Security>();

    return Security::pInstance;
}

void Security::PrintCapabilities(std::uint32_t capabilities) {
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

std::uint64_t Security::IdentityToCapabilities(const std::uint32_t identity) {
    std::uint64_t capabilities = 0x3FFFF00 | (1ll << 46ll); // Basic capability | Checkcaller check

    if (const auto capabilitiesForIdentity = identityCapabilities.find(identity);
        capabilitiesForIdentity != identityCapabilities.end()) {
        for (const auto &capability: capabilitiesForIdentity->second) {
            if (auto it = allCapabilities.find(capability); it != allCapabilities.end()) {
                if (it->second.second == true) {
                    capabilities |= (1 << it->second.first);
                    continue;
                }

                capabilities |= it->second.first;
            } else {
                Logger::GetSingleton()->PrintWarning(
                        RbxStu::Security, std::format("Couldn't find capability {} in allCapabilities", capability));
            }
        }
    }

    return capabilities;
}

void Security::SetThreadSecurity(lua_State *L, std::int32_t identity) {
    if (!Utilities::IsPointerValid(static_cast<RBX::Lua::ExtraSpace *>(L->userdata)))
        L->global->cb.userthread(L->global->mainthread,
                                 L); // If unallocated, then we must run the callback to create a valid RobloxExtraSpace

    auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
    auto capabilities = Security::GetSingleton()->IdentityToCapabilities(identity);
    plStateUd->identity = identity;
    plStateUd->capabilities = capabilities;
}

static void set_proto(Proto *proto, std::uint64_t *proto_identity) {
    // NEVER FORGET TO SET THE PROTOS and SUB PROTOS USERDATA!!
    proto->userdata = static_cast<void *>(proto_identity);
    for (auto i = 0; i < proto->sizep; i++) {
        set_proto(proto->p[i], proto_identity);
    }
}

bool Security::IsOurThread(lua_State *L) {
    /// The way we currently have of checking if a thread is our thread is through... capabilities!
    /// Roblox handles capabilities using an std::int64_t, giving us 47 fun bits to play around with.
    /// This way, we can set the bit 47th, used to describe NOTHING, to set it as
    /// our thread. Then we & it to validate it is present on the integer with an AND, which it shouldn't be ever if
    /// its anything normal, but we aren't normal!
    /// When doing the bit shift, in C by default numbers are int32_t, we must use int64_t, thus we must post-fix the
    /// the number with 'll'
    const auto extraSpace = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
    const auto logger = Logger::GetSingleton();
    const auto passed = (extraSpace->capabilities & (1ll << 46ll)) == (1ll << 46ll);
    return passed;
}

bool Security::SetLuaClosureSecurity(Closure *lClosure, std::uint32_t identity) {
    if (lClosure->isC)
        return false;
    const auto pProto = lClosure->l.p;
    auto *pMem = pProto->userdata != nullptr ? static_cast<std::uint64_t *>(pProto->userdata)
                                             : static_cast<std::uint64_t *>(malloc(sizeof(std::uint64_t)));

    *pMem = Security::GetSingleton()->IdentityToCapabilities(identity);
    set_proto(pProto, pMem);
    return true;
}

void walk_proto_wipe(lua_State *L, Proto *proto) {
    proto->debugname = nullptr;
    proto->source = luaS_new(L, "");
    proto->linedefined = -1;
    for (int i = 0; i < proto->sizep; i++) {
        walk_proto_wipe(L, proto->p[i]);
    }
}

void Security::WipeClosurePrototype(lua_State *L, const Closure *lClosure) {
    if (lClosure->isC)
        return;

    walk_proto_wipe(L, lClosure->l.p);
}
