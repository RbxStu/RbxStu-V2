//
// Created by Dottik on 11/8/2024.
//

#pragma once
#include <map>
#include <memory>
#include <mutex>
#include "Scanner.hpp"

class RobloxManager final {
private:
    static std::shared_ptr<RobloxManager> pInstance;

    bool m_bInitialized;
    std::map<std::string, void *> m_mapRobloxFunctions;

    void Initialize();

public:
    static std::shared_ptr<RobloxManager> GetSingleton();
};

namespace RbxStu {
#define MakeSignature_FromIDA(signatureName, idaSignature)                                                             \
    const static Signature signatureName = SignatureByte::GetSignatureFromIDAString(idaSignature)

    namespace Signatures {
        MakeSignature_FromIDA(RBX_ScriptContext_scriptStart,
                              "0F 57 C0 66 0F 7F 44 24 ? 48 8B 42 ? 48 85 C0 74 08 F0 FF 40 ? 48 8B 42 ? 48 8B 0A 48 "
                              "89 4C 24 ? 48 89 44 24 ? 48 85 C9 74 4D");
        MakeSignature_FromIDA(RBX_ScriptContext_openStateImpl, "33 FF 89 7C 24 ? 33 D2 48 8D 0D ? ? ? ? E8 ? ? ? ?");
        MakeSignature_FromIDA(
                RBX_ExtraSpace_initialize,
                "48 89 4C 24 ? 53 55 56 57 41 56 41 57 48 83 EC ? 48 8B D9 45 33 FF 4C 89 39 4C 89 79 ? 4C 89 79 ? 4C "
                "89 79 ? 4C 89 79 ? 48 8B 42 ? 48 85 C0 74 04 F0 FF 40 ? 48 8B 42 ? 48 89 41 ? 48 8B 42 ? 48 89 41 ? "
                "48 8B 42 ? 48 89 41 ? 48 85 C0 74 03 F0 FF 00 0F 10 42 ? 0F 11 41 ? F2 0F 10 4A ? F2 0F 11 49 ? 48 8B "
                "42 ? 48 89 41 ? 4C 89 79 ? 4C 89 79 ? 48 83 7A 58 00 74 14");
    } // namespace Signatures

#undef MakeSignature_FromIDA
} // namespace RbxStu
