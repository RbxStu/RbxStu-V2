//
// Created by Dottik on 11/8/2024.
//

#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include "Roblox/TypeDefinitions.hpp"
#include "Scanner.hpp"
#include "lua.h"

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

        using r_RBX_Console_StandardOut = std::int32_t(__fastcall *)(std::int32_t dwMessageId,
                                                                     const char *szFormatString, ...);

    } // namespace FunctionDefinitions

    namespace Signatures {
        MakeSignature_FromIDA(RBX_ScriptContext_scriptStart,
                              "48 89 54 24 ? 48 89 4C 24 ? 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B FA "
                              "4C 8B E9 0F 57 C0 66 0F 7F 44 24 ? 48 8B 42 ? 48 85 C0 74 08 F0 FF 40 ? 48 8B 42 ? 48 "
                              "8B 0A 48 89 4C 24 ? 48 89 44 24 ? 48 85 C9 74 4D");
        MakeSignature_FromIDA(
                RBX_ScriptContext_openStateImpl,
                "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? "
                "? ? 41 8B F1 45 8B E0 4C 8B EA 4C 8B F9 33 FF 89 7C 24 ? 33 D2 48 8D 0D ? ? ? ? E8 ? ? ? ?");
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
        MakeSignature_FromIDA(RBX_ScriptContext_getGlobalState,
                              "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F8 48 8B F2 48 8B D9 80 ?? ?? ?? ?? ?? "
                              "?? 74 ?? 8B 81 ? ? ? ? 90 83 F8 03 7C 0F ?? ?? ?? ?? ?? ?? ?? 33 C9 E8 ? ? ? ? 90 ?? ?? "
                              "?? ?? ?? ?? ?? 4C 8B C7 48 8B D6 E8 ? ? ? ? 48 05 88 00 00 00");

        MakeSignature_FromIDA(RBX_Security_IdentityToCapability, "48 63 01 83 F8 0A 77 3C");

        MakeSignature_FromIDA(RBX_ProximityPrompt_onTriggered,
                              "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 4C 8B F9 "
                              "E8 ? ? ? ? 48 "
                              "8B F8 48 85 C0 0F 84 C6 02 00 ? 48 8B 50 ? 48 85 D2 0F 84 D4 02 00 ? 8B 42 ? 85 C0 0F "
                              "84 C9 02 00 ?");

        MakeSignature_FromIDA(RBX_ScriptContext_task_defer,
                              "48 8B C4 48 89 58 ? 55 56 57 41 56 41 57 48 83 EC ? 48 8B E9 33 FF 48 89 78 ? 4C 8D 48 "
                              "? 4C 8D 05 ? ? ? ? 33 D2 E8 ? ? ? ? 44 8B F0 48 8B CD E8 ? ? ? ? 48 8B D8 40 32 F6");

        MakeSignature_FromIDA(
                RBX_ScriptContext_task_spawn,
                "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 33 FF 48 89 7C 24 ? 4C 8D 4C 24 ? 4C 8D 05 ? ? ? ? "
                "33 D2 E8 ? ? ? ? 8B D8 48 8B CE E8 ? ? ? ? 44 8B CB 4C 8D 44 24 ? 48 8D 54 24 ? 48 8B C8 E8 ? ? ? ?");

        MakeSignature_FromIDA(RBX_Console_StandardOut,
                              "48 8B C4 48 89 50 ? 4C 89 40 ? 4C 89 48 ? 53 48 83 EC ? 8B D9 4C 8D 40 ? 48 8D 48 ? E8 "
                              "? ? ? ? 90 33 C0 48 89 44 24 ? 48 C7 44 24 ? ? ? ? ? 48 89 44 24 ? 88 44 24 ?s");

        static const std::map<std::string, Signature> s_signatureMap = {
                {"RBX::ScriptContext::scriptStart", RBX_ScriptContext_scriptStart},
                {"RBX::ScriptContext::openStateImpl", RBX_ScriptContext_openStateImpl},
                {"RBX::ScriptContext::getGlobalState", RBX_ScriptContext_getGlobalState},
                {"RBX::ScriptContext::task_defer", RBX_ScriptContext_task_defer},
                {"RBX::ScriptContext::task_spawn", RBX_ScriptContext_task_spawn},
                {"RBX::ExtraSpace::initializeFrom", RBX_ExtraSpace_initializeFrom},
                {"RBX::Security::IdentityToCapability", RBX_Security_IdentityToCapability},
                {"RBX::ProximityPrompt::onTriggered", RBX_ProximityPrompt_onTriggered},
                {"RBX::Console::StandardOut", RBX_Console_StandardOut},
        };

    } // namespace Signatures

#undef MakeSignature_FromIDA
} // namespace RbxStu


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

    RbxStu::FunctionDefinitions::r_RBX_Console_StandardOut GetRobloxPrint();
};
