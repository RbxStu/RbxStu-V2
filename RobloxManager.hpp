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
        using r_RBX_Instance_pushInstance = void(__fastcall *)(lua_State *L, void *instance);
        using r_RBX_ProximityPrompt_onTriggered = void(__fastcall *)(void *proximityPrompt);
        using r_RBX_ScriptContext_scriptStart = void(__fastcall *)(void *scriptContext, void *baseScript);
        using r_RBX_ScriptContext_openStateImpl = bool(__fastcall *)(void *scriptContext, void *unk_0,
                                                                     std::int32_t unk_1, std::int32_t unk_2);
        using r_RBX_ExtraSpace_initializeFrom = void *(__fastcall *) (void *newExtraSpace, void *baseExtraSpace);

        using r_RBX_ScriptContext_getGlobalState = lua_State *(__fastcall *) (void *scriptContext,
                                                                              const uint64_t *identity,
                                                                              const uint64_t *unk_0);

        using r_RBX_Console_StandardOut = std::int32_t(__fastcall *)(RBX::Console::MessageType dwMessageId,
                                                                     const char *szFormatString, ...);

        using r_RBX_ScriptContext_resumeDelayedThreads = void *(__fastcall *) (void *scriptContext);

        using r_RBX_DataModel_getStudioGameStateType = RBX::DataModelType(__fastcall *)(void *dataModel);
        using r_RBX_DataModel_doCloseDataModel = void(__fastcall *)(void *dataModel);
        using r_RBX_ScriptContext_getDataModel = RBX::DataModel *(__fastcall *) (void *scriptContext);

        using r_RBX_ScriptContext_resume = void(__fastcall *)(void *scriptContext, std::int64_t unk[0x2],
                                                              RBX::Lua::WeakThreadRef **ppWeakThreadRef, int32_t nRet,
                                                              bool isError, char const *szErrorMessage);
        using r_RBX_BasePart_getNetworkOwner =
                RBX::SystemAddress *(__fastcall *) (void *basePart, RBX::SystemAddress *returnSystemAddress);
        using r_RBX_Player_findPlayerWithAddress =
                std::shared_ptr<void> *(__fastcall *) (std::shared_ptr<void> *__return,
                                                       const RBX::SystemAddress *playerAddress, const void *context);

    } // namespace StudioFunctionDefinitions

    namespace StudioSignatures {
        MakeSignature_FromIDA(
                RBX_RBXCRASH,
                "48 89 5C 24 ? 48 89 7C 24 ? 48 89 4C 24 ? 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B FA 48 8B D9 "
                "48 8B 05 ? ? ? ? 48 85 C0 74 0A FF D0 84 C0 0F 84 D0 04 00 ? E8 ? ? ? ? 85 C0 0F 84 C3 04 00 ?");

        /**
         *  @brief Search for "Script Start".
         **/
        MakeSignature_FromIDA(RBX_ScriptContext_scriptStart,
                              "48 89 54 24 ? 48 89 4C 24 ? 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B ? "
                              "4C 8B ? 0F 57 C0 66 0F 7F 44 24 ? 48 8B 42 ? 48 85 C0 74 08 F0 FF 40 ? 48 8B 42 ? 48 8B "
                              "0A 48 89 4C 24 ? 48 89 44 24 ? 48 85 C9 74 4D");
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
         *  @brief ScriptContext's GetGlobalState. This function will return the L->global->mainthread executing Luau
         *  code on the given ScriptContext. The return of this function is decrypted on call.
         **/
        MakeSignature_FromIDA(RBX_ScriptContext_getGlobalState,
                              "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F8 48 8B F2 48 8B D9 80 ?? ?? ?? ?? ?? "
                              "?? 74 ?? 8B 81 ? ? ? ? 90 83 F8 03 7C 0F ?? ?? ?? ?? ?? ?? ?? 33 C9 E8 ? ? ? ? 90 ?? ?? "
                              "?? ?? ?? ?? ?? 4C 8B C7 48 8B D6 E8 ? ? ? ? 48 05 88 00 00 00");

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
                              "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 4C 8B ?? 80 3D ?? "
                              "?? ?? ?? ?? 74 ?? 80 3D ?? ?? ?? ?? ?? 74 ?? 48 8B ");

        MakeSignature_FromIDA(RBX_Console_StandardOut,
                              "48 8B C4 48 89 50 ? 4C 89 40 ? 4C 89 48 ? 53 48 83 EC ? 8B D9 4C 8D 40 ? 48 8D 48 ? E8 "
                              "? ? ? ? 90 33 C0 48 89 44 24 ? 48 C7 44 24 ? ? ? ? ? 48 89 44 24 ? 88 44 24 ?s");

        /**
         *  @brief Search for "GameStateType" (_Edit, _PlayClient, ...) and search the if statement chain that makes
         *numbers to strings. Look at xrefs, one of them will be a direct call to getStudioGameStateType.
         *
         *  @remarks This function does not seem to require DataModels' pointer encryption.
         **/
        MakeSignature_FromIDA(RBX_DataModel_getStudioGameStateType, "8B 81 68 04 00 00 C3 CC CC CC CC");

        MakeSignature_FromIDA(RBX_DataModel_doDataModelClose,
                              "40 53 48 83 ec ?? 80 3D ?? ?? ?? ?? 00 48 8b d9 74 ?? 80 3d ?? ?? ?? ?? 00 74 ?? 48 8B "
                              "?? ?? ?? ?? ?? 3C 06 72 ?? 48 C1 E8 08 3C 03 72 ?? EB ?? 80 3D ?? ?? ?? ?? 00 74 21 0f "
                              "10 ?? ?? ?? ?? ?? 4C 8B 41 08");

        MakeSignature_FromIDA(RBX_Instance_removeAllChildren,
                              "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 48 8B 41 ? 48 85 C0 74 70 66 66 0F 1F 84 00 00 00 "
                              "00 00 48 8B 48 ? 48 8B 59 ? 48 85 DB 74 08");

        /**
         *  @brief First function call in `RBX::Instance::removeAllChildren`.
         **/
        MakeSignature_FromIDA(RBX_Instance_remove, "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 48 8B "
                                                   "D9 E8 ? ? ? ? 48 85 C0 74 1B 80 B8 49 05 00 00 00 75 12");

        MakeSignature_FromIDA(RBX_ScriptContext_setThreadIdentityAndSandbox,
                              "48 89 5C 24 ? 55 56 41 54 41 56 41 57 48 83 EC ? 45 33 F6 4D 8B F8 44 38 35 ? ? ? 06 "
                              "4C 8B E2 48 8B D9 44 89 B4 24 90 00 00 ? 41 8D 76 ? 74 07");

        MakeSignature_FromIDA(LuaVM_Load, "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D ? ? ? 48 81 EC ? ? ? "
                                          "? 4D 8B E1 49 8B D8 4C 8B EA");

        MakeSignature_FromIDA(RBX_ScriptContext_getDataModel,
                              "48 83 EC ? 48 85 C9 74 72 48 89 7C 24 ? 48 8B 79 ? 48 85 FF 74 22");

        MakeSignature_FromIDA(RBX_ScriptContext_resume,
                              "48 8B C4 44 89 48 ? 4C 89 40 ? 48 89 50 ? 48 89 48 ? 53 56 57 41 54 41 55 41 56 41 57 "
                              "48 81 EC ? ? ? ? 0F 29 70 ? 4D 8B E8 48 8B F2 48 8B F9 48 89 8C 24 ? ? ? ? 48 89 8C 24 "
                              "? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 89 44 24 ?");

        MakeSignature_FromIDA(
                RBX_ScriptContext_validateThreadAccess,
                "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B DA 48 8B E9 80");

        MakeSignature_FromIDA(
                RBX_Instance_pushInstance,
                "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B FA 48 8B D9 48 83 3A 00 74 5E 48 8B D1 48 8D 4C 24 ? "
                "E8 ? ? ? ? 90 4C 8B 07 48 8D 54 24 ? 48 8B 4C 24 ? E8 ? ? ? ? 0F B6 F0 48 8B 4C 24 ? 48 85 C9 74 15");

        MakeSignature_FromIDA(RBX_BasePart_getNetworkOwner, "48 8B 81 ? ? ? ? 8B 88 ? ? ? ? 48 8B C2 89 0A C3");

        MakeSignature_FromIDA(RBX_Players_findPlayerWithAddress,
                              "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ? 4C 8B EA 4C 8B F9 4D 85 C0 0F "
                              "84 AE 02 00 ? 49 8B 78 ? 48 85 FF 74 13 48 8B 4F ? 48 85 C9 74 0D E8 ? ? ? ? 48 8B F8");

        MakeSignature_FromIDA(RBX_Instance_getTopAncestor,
                              "48 8B 41 ? 48 85 C0 74 08 48 8B C8 E9 EF FF FF FF 48 8B C1 C3 CC CC");

        static const std::map<std::string, Signature> s_signatureMap = {
                {"RBX::ScriptContext::resumeDelayedThreads", RBX_ScriptContext_resumeDelayedThreads},
                {"RBX::ScriptContext::scriptStart", RBX_ScriptContext_scriptStart},
                {"RBX::ScriptContext::openStateImpl", RBX_ScriptContext_openStateImpl},
                {"RBX::ScriptContext::getGlobalState", RBX_ScriptContext_getGlobalState},
                {"RBX::ScriptContext::task_wait", RBX_ScriptContext_task_wait},
                {"RBX::ScriptContext::task_defer", RBX_ScriptContext_task_defer},
                {"RBX::ScriptContext::task_spawn", RBX_ScriptContext_task_spawn},
                {"RBX::ScriptContext::task_delay", RBX_ScriptContext_task_delay},
                {"RBX::ScriptContext::getDataModel", RBX_ScriptContext_getDataModel},
                {"RBX::ScriptContext::setThreadIdentityAndSandbox", RBX_ScriptContext_setThreadIdentityAndSandbox},
                {"RBX::ScriptContext::resume", RBX_ScriptContext_resume},
                {"RBX::ScriptContext::validateThreadAccess", RBX_ScriptContext_validateThreadAccess},

                {"RBX::RBXCRASH", RBX_RBXCRASH},

                {"RBX::ExtraSpace::initializeFrom", RBX_ExtraSpace_initializeFrom},

                {"RBX::ProximityPrompt::onTriggered", RBX_ProximityPrompt_onTriggered},

                {"RBX::Console::StandardOut", RBX_Console_StandardOut},

                {"RBX::Instance::removeAllChildren", RBX_Instance_removeAllChildren},
                {"RBX::Instance::remove", RBX_Instance_remove},
                {"RBX::Instance::pushInstance", RBX_Instance_pushInstance},
                {"RBX::Instance::getTopAncestor", RBX_Instance_getTopAncestor},

                {"RBX::BasePart::getNetworkOwner", RBX_BasePart_getNetworkOwner},
                {"RBX::Players::findPlayerWithAddress", RBX_Players_findPlayerWithAddress},

                {"RBX::DataModel::doDataModelClose", RBX_DataModel_doDataModelClose},
                {"RBX::DataModel::getStudioGameStateType", RBX_DataModel_getStudioGameStateType},

                {"LuaVM::Load", LuaVM_Load},
        };

    } // namespace StudioSignatures

#undef MakeSignature_FromIDA
} // namespace RbxStu


/// @brief Manages the way RbxStu interacts with Roblox specific internal functions.
class RobloxManager final {
    /// @brief Private, Static shared pointer into the instance.
    static std::shared_ptr<RobloxManager> pInstance;

    /// @brief The map for Roblox functions. May include hooked functions.
    std::map<std::string, void *> m_mapRobloxFunctions;

    /// @brief The map for the original Roblox functions for when a Roblox functions is hooked. Guaranteed to not
    /// contain any hooked functions
    std::map<std::string, void *> m_mapHookMap;

    /// @brief The map used to keep track of the valid RBX::DataModel pointers.
    std::map<RBX::DataModelType, RBX::DataModel *> m_mapDataModelMap;

    /// @brief Whether the current instance is initialized.
    bool m_bInitialized = false;

    /// @brief The map used to hold scanned data pointers.
    std::map<std::string, void *> m_mapDataPointersMap;

    /// @brief The map used to hold the name and the reference that points to the fast variables in roblox.
    std::map<std::string, void *> m_mapFastVariables;

    /// @brief The map used to hold the pointer's encryption type and the identifier.
    std::map<std::string, RBX::PointerEncryptionType> m_mapPointerEncryptionMap;

    /// @brief The map used to hold the pointer's encryption and offset.
    std::map<std::string, std::pair<std::uintptr_t, RBX::PointerEncryptionType>> m_mapPointerOffsetEncryption;

    /// @brief Initializes the RobloxManager instance, obtaining all functions from their respective signatures and
    /// establishing the initial hooks required for the manager to operate as expected.
    void Initialize();

public:
    /// @brief Obtains the shared pointer that points to the global singleton for the current class.
    /// @return Singleton for RobloxManager as a std::shared_ptr<RobloxManager>.
    static std::shared_ptr<RobloxManager> GetSingleton();

    /// @brief Attempts to obtain the L->global->mainthread for the given ScriptContext instance.
    /// @param ScriptContext [in] A pointer in memory towards a valid ScriptContext instance.
    /// @return A std::optional<lua_State *> which pointers to a non-array lua_State *, which is the ScriptContext's
    /// main lua_State thread.
    /// @remarks This function does not obtain the DataModel of ScriptContext, it is up to the callers responsability to
    /// obtain the correct ScriptContext to obtain the correct lua_State. This function may also fail to execute, in
    /// case that RBX::ScriptContext::getGlobalState cannot be found on the Roblox Studio assembly via AOB search.
    std::optional<lua_State *> GetGlobalState(_In_ void *ScriptContext);

    /// @brief Obtains the print address for the Roblox console. Used to print to the Roblox Developer Console.
    /// @return A std::optional<...> which holds the pointer to the Developer Console's standard output.
    /// @remarks Calling this function is unsafe if the optional type is not checked for its validity!
    std::optional<RbxStu::StudioFunctionDefinitions::r_RBX_Console_StandardOut> GetRobloxPrint();

    std::optional<lua_CFunction> GetRobloxTaskDefer();

    std::optional<lua_CFunction> GetRobloxTaskSpawn();

    std::optional<void *> GetScriptContext(const lua_State *L) const;

    std::optional<RBX::DataModel *> GetDataModelFromScriptContext(void *scriptContext);

    /// @brief Returns whether this instance of RobloxManager is initialized.
    /// @return Whether the instance is initialized to completion.
    bool IsInitialized() const;

    /// @brief Obtains a Roblox function from the list of functions that were found successfully via scanning.
    /// @remark The returned function may be hooked by RobloxManager to add functionality. In case you wish for the
    /// ORIGINAL function, use GetHookOriginal(const std::string &functionName) instead!
    /// @return A type-less pointer into the start of the function.
    void *GetRobloxFunction(const std::string &functionName);

    /// @brief Resumes a lua_State through the Roblox scheduler
    /// @param threadRef The thread reference representing the state of the thread for yielding
    /// @param nret The number of returns after yielding.
    /// @param isError Whether the resumption is an error
    /// @param szErrorMessage The message of the error; only applies when isError is true.
    void ResumeScript(RBX::Lua::WeakThreadRef *threadRef, std::int32_t nret, bool isError, const char *szErrorMessage);

    /// @brief Obtains the original function given the function's name.
    /// @param functionName The name of the Roblox function to obtain the original from.
    /// @note This function may return non-hooked functions as well.
    /// @remark WARNING ON USAGE: This function is for internal usage of RobloxManager, whilst callers may use
    /// it to obtain the original version of a Roblox function on the remote Roblox environment or to hook it
    /// themselves, this is discouraged, and wrong, do NOT do that.
    /// @return A type-less pointer into the start of the original function.
    void *GetHookOriginal(const std::string &functionName);

    /// @brief Obtains the most recient and up-to-date DataModel for the given DataModel type.
    /// @param type Describes the type of DataModel to check for.
    /// @return A std::optional<RBX::DataModel *> into an instance of a DataModel which is of the type described on the
    /// parameter.
    std::optional<RBX::DataModel *> GetCurrentDataModel(const RBX::DataModelType &type) const;

    /// @brief Updates the current DataModel pointer for the given type.
    /// @param dataModelType Describes the type of DataModel to check for.
    /// @param dataModel [in] The pointer to the RBX::DataModel structure to replace the current one for.
    /// @remark This function will not accept DataModel structures whose m_bIsClosed field is set to true, this is to
    /// guarantee RobloxManager's behaviour.
    /// @return A std::optional<RBX::DataModel *> into an instance of a DataModel which is of the type described on the
    /// parameter.
    void SetCurrentDataModel(const RBX::DataModelType &dataModelType, _In_ RBX::DataModel *dataModel);

    /// @brief Validates the DataModel for the given DataModelType.
    /// @param type Describes the type of DataModel to check the validity of.
    /// @remark This function is meant for internal usage. It is exposed with the reason that it may be useful to
    /// callers to validate any RBX::DataModel instance that may have been obtained.
    /// @return A boolean that describes if the following is true valid:
    ///     - The pointer is on an allocated memory page.
    ///     - The DataModel has not been closed ((RBX::DataModel *)->m_bIsClosed).
    bool IsDataModelValid(const RBX::DataModelType &type) const;
    std::optional<void *> GetFastVariable(const std::string &str);
};
