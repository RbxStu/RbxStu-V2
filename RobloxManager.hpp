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

    namespace StudioFunctionDefinitions {
        using r_RBX_ScriptContext_scriptStart = void(__fastcall *)(void *scriptContext, void *baseScript);
        using r_RBX_ScriptContext_openStateImpl = bool(__fastcall *)(void *scriptContext, void *unk_0,
                                                                     std::int32_t unk_1, std::int32_t unk_2);
        using r_RBX_ExtraSpace_initializeFrom = void *(__fastcall *) (void *newExtraSpace, void *baseExtraSpace);
        using r_RBX_ScriptContext_getGlobalState = lua_State *(__fastcall *) (void *scriptContext, void *identity,
                                                                              void *unk_0);
        using r_RBX_Security_IdentityToCapability = std::int64_t(__fastcall *)(std::int32_t *pIdentity);

        using r_RBX_Console_StandardOut = std::int32_t(__fastcall *)(RBX::Console::MessageType dwMessageId,
                                                                     const char *szFormatString, ...);

        using r_RBX_ScriptContext_resumeDelayedThreads = void *(__fastcall *) (void *scriptContext);

        using r_RBX_DataModel_getStudioGameStateType = RBX::DataModelType(__fastcall *)(void *dataModel);
    } // namespace StudioFunctionDefinitions

    namespace StudioSignatures {
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

        MakeSignature_FromIDA(RBX_ScriptContext_task_delay,
                              "48 89 5C 24 ? 55 56 57 41 56 41 57 48 83 EC ? 0F 29 74 24 ? 0F 29 7C 24 ? 4C "
                              "8B F1 0F 57 F6 0F 57 D2 BA ? ? ? ? E8 ? ? ? ? 0F 28 F8 33 FF 8D 5F ? 66 0F 2F F0 77 55 "
                              "0F 57 C0 F2 0F 5A C7 E8 ? ? ? ?F");

        MakeSignature_FromIDA(RBX_ScriptContext_task_wait,
                              "48 89 5C 24 ? 55 56 57 48 83 EC ? 0F 29 74 24 ? 0F 29 7C 24 ? 48 8B D9 E8 ? ? ? ? 85 C0 "
                              "0F 84 89 01 00 ? 0F 57 F6 0F 57 D2 BA ? ? ? ? 48 8B CB E8 ? ? ? ? 0F 28 F8 33 FF 66 0F "
                              "2F F0 77 56 0F 57 C0 F2 0F 5A C7");

        MakeSignature_FromIDA(RBX_ScriptContext_resumeDelayedThreads,
                              "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 4C 8B F1 80 3D ?? "
                              "?? ?? ?? ?? 74 ?? 80 3D ?? ?? ?? ?? ?? 74 ?? 48 8B ");

        MakeSignature_FromIDA(RBX_Console_StandardOut,
                              "48 8B C4 48 89 50 ? 4C 89 40 ? 4C 89 48 ? 53 48 83 EC ? 8B D9 4C 8D 40 ? 48 8D 48 ? E8 "
                              "? ? ? ? 90 33 C0 48 89 44 24 ? 48 C7 44 24 ? ? ? ? ? 48 89 44 24 ? 88 44 24 ?s");

        MakeSignature_FromIDA(RBX_DataModel_getStudioGameStateType, "8b 81 60 04 00 00 C3 CC CC CC CC");

        static const std::map<std::string, Signature> s_signatureMap = {
                {"RBX::DataModel::getStudioGameStateType", RBX_DataModel_getStudioGameStateType},
                {"RBX::ScriptContext::resumeDelayedThreads", RBX_ScriptContext_resumeDelayedThreads},
                {"RBX::ScriptContext::scriptStart", RBX_ScriptContext_scriptStart},
                {"RBX::ScriptContext::openStateImpl", RBX_ScriptContext_openStateImpl},
                {"RBX::ScriptContext::getGlobalState", RBX_ScriptContext_getGlobalState},
                {"RBX::ScriptContext::task_wait", RBX_ScriptContext_task_wait},
                {"RBX::ScriptContext::task_defer", RBX_ScriptContext_task_defer},
                {"RBX::ScriptContext::task_spawn", RBX_ScriptContext_task_spawn},
                {"RBX::ScriptContext::task_delay", RBX_ScriptContext_task_delay},
                {"RBX::ExtraSpace::initializeFrom", RBX_ExtraSpace_initializeFrom},
                {"RBX::Security::IdentityToCapability", RBX_Security_IdentityToCapability},
                {"RBX::ProximityPrompt::onTriggered", RBX_ProximityPrompt_onTriggered},
                {"RBX::Console::StandardOut", RBX_Console_StandardOut},
        };

    } // namespace StudioSignatures

#undef MakeSignature_FromIDA
} // namespace RbxStu


class RobloxManager final {
private:
    static std::shared_ptr<RobloxManager> pInstance;

    bool m_bInitialized;
    std::map<std::string, void *> m_mapRobloxFunctions;
    std::map<std::string, void *> m_mapHookMap;

    // Roblox fields.
    std::map<RBX::DataModelType, void *> m_mapDataModelMap;

    void Initialize();

public:
    static std::shared_ptr<RobloxManager> GetSingleton();

    std::optional<lua_State *> GetGlobalState(_In_ void *ScriptContext);

    std::optional<std::int64_t> IdentityToCapability(std::int32_t identity);

    RbxStu::StudioFunctionDefinitions::r_RBX_Console_StandardOut GetRobloxPrint();

    bool IsInitialized() const;

    void *GetRobloxFunction(const std::string &functionName);

    void *GetHookOriginal(const std::string &functionName);

    void *GetCurrentDataModel(RBX::DataModelType type) const;
    void SetCurrentDataModel(RBX::DataModelType type, _In_ void *dataModel);
};
