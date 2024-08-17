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
    plStateUd->capabilities = 0x3FFFF00 | RobloxManager::GetSingleton()->IdentityToCapability(8).value();
    // Magical constant | Custom_Identity (Or Capabilities in some cases)
}

static void set_proto(Proto *proto, uintptr_t *proto_identity) {
    // NEVER FORGET TO SET THE PROTOS and SUB PROTOS USERDATA!!
    proto->userdata = static_cast<void *>(proto_identity);
    for (auto i = 0; i < proto->sizep; i++)
        set_proto(proto->p[i], proto_identity);
}

bool Security::SetLuaClosureSecurity(Closure *lClosure) {
    if (lClosure->isC)
        return false;
    const auto pProto = lClosure->l.p;
    auto *pMem = pProto->userdata != nullptr ? static_cast<std::uintptr_t *>(pProto->userdata)
                                             : static_cast<std::uintptr_t *>(malloc(sizeof(std::uintptr_t)));

    *pMem = 0x3FFFF00 | RobloxManager::GetSingleton()->IdentityToCapability(8).value();
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
