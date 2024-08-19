//
// Created by Dottik 16/8/2024.
//

#include "Security.hpp"
#include <cstdint>
#include <cstdio>
#include <lstate.h>

#include "RobloxManager.hpp"
#include "Utilities.hpp"

std::shared_ptr<Security> Security::pInstance;

std::shared_ptr<Security> Security::GetSingleton() {
    if (Security::pInstance == nullptr)
        Security::pInstance = std::make_shared<Security>();

    return Security::pInstance;
}

void Security::SetThreadSecurity(lua_State *L) {
    if (!Utilities::IsPointerValid(static_cast<RBX::Lua::ExtraSpace *>(L->userdata)))
        L->global->cb.userthread(L->global->mainthread,
                                 L); // If unallocated, then we must run the callback to create a valid RobloxExtraSpace

    auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);

    plStateUd->identity = 8;
    plStateUd->capabilities = 0x3FBF897F | 0b100000000; // Find the Capability To String, and OR them all together.
    // Magical constant | Custom_Identity (Or Capabilities in some cases)
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
