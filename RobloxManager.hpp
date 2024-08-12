//
// Created by Dottik on 11/8/2024.
//

#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include "Scanner.hpp"
#include "lua.h"

class RobloxManager final {
private:
    static std::shared_ptr<RobloxManager> pInstance;

    bool m_bInitialized;
    std::map<std::string, void *> m_mapRobloxFunctions;

    void Initialize();

public:
    static std::shared_ptr<RobloxManager> GetSingleton();

    std::optional<lua_State *> GetGlobalState(_In_ void *ScriptContext);

    std::optional<std::int64_t> IdentityToCapability(std::int32_t identity);
};

namespace RbxStu {
#define MakeSignature_FromIDA(signatureName, idaSignature)                                                             \
    const static Signature signatureName = SignatureByte::GetSignatureFromIDAString(idaSignature)

    namespace FunctionDefinitions {
        using r_RBX_ScriptContext_scriptStart = void(__fastcall *)(void *scriptContext, void *baseScript);
        using r_RBX_ScriptContext_openStateImpl = bool(__fastcall *)(void *scriptContext, void *unk_0,
                                                                     std::int32_t unk_1, std::int32_t unk_2);
        using r_RBX_ExtraSpace_initializeFrom = void *(__fastcall *) (void *newExtraSpace, void *baseExtraSpace);
        using r_RBX_ScriptContext_getGlobalState = lua_State *(__fastcall *) (void *scriptContext, void *identity,
                                                                              void *unk_0);
        using r_RBX_Security_IdentityToCapability = std::int64_t(__fastcall *)(std::int32_t *pIdentity);

    } // namespace FunctionDefinitions

    namespace Signatures {
        MakeSignature_FromIDA(RBX_ScriptContext_scriptStart,
                              "0F 57 C0 66 0F 7F 44 24 ? 48 8B 42 ? 48 85 C0 74 08 F0 FF 40 ? 48 8B 42 ? 48 8B 0A 48 "
                              "89 4C 24 ? 48 89 44 24 ? 48 85 C9 74 4D");
        MakeSignature_FromIDA(RBX_ScriptContext_openStateImpl, "33 FF 89 7C 24 ? 33 D2 48 8D 0D ? ? ? ? E8 ? ? ? ?");
        MakeSignature_FromIDA(
                RBX_ExtraSpace_initializeFrom,
                "48 89 4C 24 ? 53 55 56 57 41 56 41 57 48 83 EC ? 48 8B D9 45 33 FF 4C 89 39 4C 89 79 ? 4C 89 79 ? 4C "
                "89 79 ? 4C 89 79 ? 48 8B 42 ? 48 85 C0 74 04 F0 FF 40 ? 48 8B 42 ? 48 89 41 ? 48 8B 42 ? 48 89 41 ? "
                "48 8B 42 ? 48 89 41 ? 48 85 C0 74 03 F0 FF 00 0F 10 42 ? 0F 11 41 ? F2 0F 10 4A ? F2 0F 11 49 ? 48 8B "
                "42 ? 48 89 41 ? 4C 89 79 ? 4C 89 79 ? 48 83 7A 58 00 74 14");
        /**
         *  @brief ScriptContext's GetGlobalState. This function will return the mainthread executing Luau code on the
         * given ScriptContext. The return of this function is "encrypted", a wrapper exists within RobloxManager to
         * decrypt it with the current method, if the method does not work, the AOB likely will not either.
         **/
        MakeSignature_FromIDA(RBX_ScriptContext_GetGlobalState,
                              "7C ?? 48 8D 15 ? ? ? ? ?? ?? ?? ? ? ? ? ?? ?? ?? ?? ? ? ? ? ?? ?? ?? ?? ?? ?? ?? ? ? ? "
                              "? ?? ?? ? ? ? ? 8B 10 03 D0 89 54 24 ? 03 40 ? 89 44 24 ? 48 8B 44 24 ? 48 8B 5C 24 ? "
                              "48 8B 74 24 ? 48 83 C4 ? 5F C3");

        MakeSignature_FromIDA(RBX_Security_IdentityToCapability, "48 63 01 83 F8 0A 77 3C");
    } // namespace Signatures

#undef MakeSignature_FromIDA
} // namespace RbxStu
