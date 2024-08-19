//
// Created by Dottik on 11/8/2024.
//

#include "RobloxManager.hpp"

#include <MinHook.h>
#include <Windows.h>
#include <shared_mutex>

#include <DbgHelp.h>

#include "LuauManager.hpp"
#include "Scanner.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "lualib.h"

std::shared_mutex __robloxmanager__singleton__lock;

void rbx_rbxcrash(const char *crashType, const char *crashDescription) {
    const auto logger = Logger::GetSingleton();

    if (crashType == nullptr)
        crashType = "Unknown";

    if (crashDescription == nullptr)
        crashDescription = "Not described";

    logger->PrintError(RbxStu::Hooked_RBXCrash, "CRASH TRIGGERED!");
    logger->PrintInformation(RbxStu::Hooked_RBXCrash, std::format("CRASH TYPE: {}", crashType));
    logger->PrintInformation(RbxStu::Hooked_RBXCrash, std::format("CRASH DESCRIPTION: {}", crashDescription));

    logger->PrintInformation(RbxStu::Hooked_RBXCrash, "Displaying stack trace!");

    SymInitialize(GetCurrentProcess(), nullptr, TRUE);

    void *stack[256];
    const unsigned short frameCount = RtlCaptureStackBackTrace(0, 100, stack, nullptr);

    for (unsigned short i = 0; i < frameCount; ++i) {
        auto address = reinterpret_cast<DWORD64>(stack[i]);
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbolBuffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD value{};
        DWORD *pValue = &value;
        if (SymFromAddr(GetCurrentProcess(), address, nullptr, symbol) && ((*pValue = symbol->Address - address)) &&
            SymFromAddr(GetCurrentProcess(), address, reinterpret_cast<PDWORD64>(pValue), symbol)) {
            printf(("[Stack Frame %d] Inside %s @ 0x%p; Studio Rebase: 0x%p\r\n"), i, symbol->Name, address,
                   address - reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxStudioBeta.exe")) + 0x140000000);
        } else {
            printf(("[Stack Frame %d] Unknown Subroutine @ 0x%p; Studio Rebase: 0x%p\r\n"), i, symbol->Name, address,
                   address - reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxStudioBeta.exe")) + 0x140000000);
        }
    }

    SymCleanup(GetCurrentProcess());
    MessageBoxA(nullptr, ("Studio Crash"), ("Execution suspended. RBXCRASH has been called."), MB_OK);

    Sleep(60000);
}

std::shared_mutex __rbx__scriptcontext__resumeWaitingThreads__lock;

/// @brief Used for the hook of RBX::ScriptContext::resumeWaitingThreads to prevent accessing uninitialized lua_States.
std::atomic_int calledBeforeCount;

void *rbx__scriptcontext__resumeWaitingThreads(
        void *waitingHybridScriptsJob) { // the "scriptContext" is actually a std::vector of waitinghybridscripts as it
                                         // seems.

    const auto robloxManager = RobloxManager::GetSingleton();
    const auto logger = Logger::GetSingleton();
    const auto scheduler = Scheduler::GetSingleton();
    const auto security = Security::GetSingleton();

    if (!robloxManager->IsInitialized())
        return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ScriptContext_resumeDelayedThreads>(
                robloxManager->GetHookOriginal("RBX::ScriptContext::resumeDelayedThreads"))(waitingHybridScriptsJob);

    __rbx__scriptcontext__resumeWaitingThreads__lock.lock();
    auto scriptContext =
            *reinterpret_cast<void **>(*reinterpret_cast<std::uintptr_t *>(waitingHybridScriptsJob) + 0x1F8);

    // logger->PrintInformation(RbxStu::HookedFunction,
    //                         std::format("ScriptContext::resumeWaitingThreads. ScriptContext: {:#x}", ScriptContext));

    if (!scheduler->IsInitialized()) { // !scheduler->is_initialized()
        auto getDataModel = reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ScriptContext_getDataModel>(
                robloxManager->GetRobloxFunction("RBX::ScriptContext::getDataModel"));
        if (getDataModel == nullptr) {
            logger->PrintWarning(RbxStu::HookedFunction, "Initialization of Scheduler may be unstable! Cannot "
                                                         "determine DataModel for the obtained ScriptContext!");
        } else {
            const auto expectedDataModel = robloxManager->GetCurrentDataModel(RBX::DataModelType_PlayClient);
            if (!expectedDataModel.has_value() || getDataModel(scriptContext) != expectedDataModel.value() ||
                !robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
                goto __scriptContext_resumeWaitingThreads__cleanup;
            }
        }

        // HACK!: We do not want to initialize the scheduler on the
        // first resumptions of waiting threads. This will cause
        // us to access invalid memory, as the global state is not truly set up yet apparently,
        // race conditions at their finest! This had to be increased, because Roblox.
        if (calledBeforeCount <= 256) {
            calledBeforeCount += 1;
            goto __scriptContext_resumeWaitingThreads__cleanup;
        }

        const auto optionalrL = robloxManager->GetGlobalState(scriptContext);
        logger->PrintWarning(RbxStu::HookedFunction,
                             std::format("WaitingHybridScriptsJob: {}", waitingHybridScriptsJob));
        logger->PrintWarning(RbxStu::HookedFunction, std::format("ScriptContext: {}", scriptContext));
        logger->PrintWarning(RbxStu::HookedFunction, std::format("ScriptContext__GlobalState: {}",
                                                                 reinterpret_cast<void *>(optionalrL.value())));


        if (optionalrL.has_value() && robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
            const auto robloxL = optionalrL.value();
            lua_State *rL = lua_newthread(robloxL);
            lua_pop(robloxL, 1);
            lua_State *L = lua_newthread(robloxL);
            lua_pop(robloxL, 1);
            security->SetThreadSecurity(rL);
            security->SetThreadSecurity(L);
            lua_pop(L, lua_gettop(L));

            scheduler->InitializeWith(L, rL, getDataModel(scriptContext));
        }
    } else if (scheduler->IsInitialized() && !robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
        logger->PrintWarning(RbxStu::HookedFunction, "DataModel for client is invalid, yet the scheduler is "
                                                     "initialized, resetting scheduler!");
        scheduler->ResetScheduler();
    }

    calledBeforeCount = 0;
__scriptContext_resumeWaitingThreads__cleanup:
    __rbx__scriptcontext__resumeWaitingThreads__lock.unlock();
    return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ScriptContext_resumeDelayedThreads>(
            robloxManager->GetHookOriginal("RBX::ScriptContext::resumeDelayedThreads"))(waitingHybridScriptsJob);
}
void rbx__datamodel__dodatamodelclose(void **dataModelContainer) {
    // DataModel = *(dataModelContainer + 0x8)
    auto dataModel = *reinterpret_cast<void **>(reinterpret_cast<std::uintptr_t>(dataModelContainer) + 0x8);

    const auto robloxManager = RobloxManager::GetSingleton();
    const auto original = reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_DataModel_doCloseDataModel>(
            robloxManager->GetHookOriginal("RBX::DataModel::doDataModelClose"));

    if (!robloxManager->IsInitialized())
        return original(dataModelContainer);

    const auto getStudioGameStateType =
            reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_DataModel_getStudioGameStateType>(
                    robloxManager->GetHookOriginal("RBX::DataModel::getStudioGameStateType"));

    const auto logger = Logger::GetSingleton();

    original(dataModelContainer);

    if (!getStudioGameStateType) {
        logger->PrintWarning(RbxStu::HookedFunction,
                             "A DataModel has been closed, but we don't know the type, as RobloxManager has failed to "
                             "obtain RBX::DataModel::getStudioGameStateType, cleaning up all significant DataModel "
                             "pointers for RobloxManager instead!");

        robloxManager->SetCurrentDataModel(RBX::DataModelType_PlayClient, nullptr);
        robloxManager->SetCurrentDataModel(RBX::DataModelType_PlayServer, nullptr);
        robloxManager->SetCurrentDataModel(RBX::DataModelType_Edit, nullptr);
        robloxManager->SetCurrentDataModel(RBX::DataModelType_MainMenuStandalone, nullptr);
        robloxManager->SetCurrentDataModel(RBX::DataModelType_Null, nullptr);
    } else {
        logger->PrintWarning(RbxStu::HookedFunction,
                             std::format("The DataModel corresponding to {} (DataModelTypeID: {}) has been closed. "
                                         "(Closed DataModel: {})",
                                         RBX::DataModelTypeToString(getStudioGameStateType(dataModel)),
                                         static_cast<std::int32_t>(getStudioGameStateType(dataModel)), dataModel));
        robloxManager->SetCurrentDataModel(getStudioGameStateType(dataModel), static_cast<RBX::DataModel *>(dataModel));
    }
}

std::int32_t rbx__datamodel__getstudiogamestatetype(RBX::DataModel *dataModel) {
    auto robloxManager = RobloxManager::GetSingleton();
    auto original = reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_DataModel_getStudioGameStateType>(
            robloxManager->GetHookOriginal("RBX::DataModel::getStudioGameStateType"));

    if (!robloxManager->IsInitialized())
        return original(dataModel);

    switch (auto dataModelType = original(dataModel)) {
        case RBX::DataModelType::DataModelType_Edit:
            if (robloxManager->GetCurrentDataModel(RBX::DataModelType::DataModelType_Edit) != dataModel) {
                auto logger = Logger::GetSingleton();
                logger->PrintInformation(RbxStu::HookedFunction, "Obtained Newest Edit Mode DataModel!");
                robloxManager->SetCurrentDataModel(RBX::DataModelType::DataModelType_Edit, dataModel);
            }
            break;
        case RBX::DataModelType_PlayClient:
            if (robloxManager->GetCurrentDataModel(RBX::DataModelType::DataModelType_PlayClient) != dataModel) {
                auto logger = Logger::GetSingleton();
                logger->PrintInformation(RbxStu::HookedFunction, "Obtained Newest Playing Client DataModel!");
                robloxManager->SetCurrentDataModel(RBX::DataModelType_PlayClient, dataModel);
            }
            break;

        case RBX::DataModelType_MainMenuStandalone:
            if (robloxManager->GetCurrentDataModel(RBX::DataModelType::DataModelType_MainMenuStandalone) != dataModel) {
                auto logger = Logger::GetSingleton();
                logger->PrintInformation(RbxStu::HookedFunction,
                                         "Obtained Newest Standalone (Main Menu of Studio) DataModel!");
                robloxManager->SetCurrentDataModel(RBX::DataModelType_MainMenuStandalone, dataModel);
            }
            break;

        case RBX::DataModelType_PlayServer:
            if (robloxManager->GetCurrentDataModel(RBX::DataModelType::DataModelType_PlayServer) != dataModel) {
                auto logger = Logger::GetSingleton();
                logger->PrintInformation(RbxStu::HookedFunction, "Obtained Newest Playing Server DataModel!");
                robloxManager->SetCurrentDataModel(RBX::DataModelType_PlayServer, dataModel);
            }
            break;
    }

    return original(dataModel);
}

std::shared_ptr<RobloxManager> RobloxManager::pInstance;

void RobloxManager::Initialize() {
    auto logger = Logger::GetSingleton();
    if (this->m_bInitialized) {
        logger->PrintWarning(RbxStu::RobloxManager, "This instance is already initialized!");
        return;
    }
    auto scanner = Scanner::GetSingleton();
    logger->PrintInformation(RbxStu::RobloxManager, "Initializing Roblox Manager [1/3]");
    logger->PrintInformation(RbxStu::RobloxManager, "Initializing MinHook for function hooking [1/3]");
    MH_Initialize();

    logger->PrintInformation(RbxStu::RobloxManager, "Scanning for functions (Simple step)... [1/3]");

    for (const auto &[fName, fSignature]: RbxStu::StudioSignatures::s_signatureMap) {
        const auto results = scanner->Scan(fSignature);

        if (results.empty()) {
            logger->PrintWarning(RbxStu::RobloxManager, std::format("Failed to find function '{}'!", fName));
        } else {
            void *mostDesirable = *results.data();
            if (results.size() > 1) {
                logger->PrintWarning(
                        RbxStu::RobloxManager,
                        std::format(
                                "More than one candidate has matched the signature. This is generally not something "
                                "problematic, but it may mean the signature has too many wildcards that makes it not "
                                "unique. "
                                "The first result will be chosen. Affected function: {}",
                                fName));

                auto closest = *results.data();
                for (const auto &result: results) {
                    if (reinterpret_cast<std::uintptr_t>(result) -
                                reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr)) <
                        (reinterpret_cast<std::uintptr_t>(closest) -
                         reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr)))) {
                        mostDesirable = result;
                    }
                }
            }
            // Here we need to get a little bit smart. It is very possible we have MORE than one result.
            // To combat this, we want to grab the address that is closest to GetModuleHandleA(nullptr).
            // A mere attempt at making THIS, less painful.

            this->m_mapRobloxFunctions[fName] =
                    mostDesirable; // Grab first result, it doesn't really matter to be honest.
        }
    }

    logger->PrintInformation(RbxStu::RobloxManager, "Functions Found via simple scanning:");
    for (const auto &[funcName, funcAddress]: this->m_mapRobloxFunctions) {
        logger->PrintInformation(RbxStu::RobloxManager, std::format("- '{}' at address {}.", funcName, funcAddress));
    }

    logger->PrintInformation(RbxStu::RobloxManager, "Additional dumping step... [2/3]");

    {
        logger->PrintInformation(RbxStu::RobloxManager, "Attempting to obtain ");
        { // getglobalstate encryption dump, both functions' encryption match correctly.

            auto functionStart = this->m_mapRobloxFunctions["RBX::ScriptContext::getGlobalState"];
            const auto asm_0 = reinterpret_cast<std::uint8_t *>(reinterpret_cast<std::uintptr_t>(functionStart) + 0x56);

            switch (*asm_0) {
                case 0x2B: // sub ecx, dword ptr [rax]
                    logger->PrintInformation(RbxStu::RobloxManager, "Encryption is SUB");
                    m_mapPointerEncryptionMap["RBX::ScriptContext::globalState"] =
                            RbxStu::RbxPointerEncryptionType::SUB;
                    break;

                case 0x3: // add ecx, dword ptr [rax]
                    logger->PrintInformation(RbxStu::RobloxManager, "Encryption is ADD");
                    m_mapPointerEncryptionMap["RBX::ScriptContext::globalState"] =
                            RbxStu::RbxPointerEncryptionType::ADD;
                    break;

                case 0x33: // xor ecx, dword ptr [rax]
                    logger->PrintInformation(RbxStu::RobloxManager, "Encryption is XOR");
                    m_mapPointerEncryptionMap["RBX::ScriptContext::globalState"] =
                            RbxStu::RbxPointerEncryptionType::XOR;
                    break;

                default:
                    logger->PrintInformation(RbxStu::RobloxManager,
                                             std::format("Encryption match failed found code: {}", *asm_0));
                    m_mapPointerEncryptionMap["RBX::ScriptContext::globalState"] =
                            RbxStu::RbxPointerEncryptionType::UNDETERMINED;
                    break;
            }
        }
    }

    logger->PrintInformation(RbxStu::RobloxManager, "Initializing hooks... [2/3]");

    this->m_mapHookMap["RBX::ScriptContext::resumeDelayedThreads"] = new void *();
    MH_CreateHook(this->m_mapRobloxFunctions["RBX::ScriptContext::resumeDelayedThreads"],
                  rbx__scriptcontext__resumeWaitingThreads,
                  &this->m_mapHookMap["RBX::ScriptContext::resumeDelayedThreads"]);
    MH_EnableHook(this->m_mapRobloxFunctions["RBX::ScriptContext::resumeDelayedThreads"]);

    this->m_mapHookMap["RBX::DataModel::getStudioGameStateType"] = new void *();
    MH_CreateHook(this->m_mapRobloxFunctions["RBX::DataModel::getStudioGameStateType"],
                  rbx__datamodel__getstudiogamestatetype,
                  &this->m_mapHookMap["RBX::DataModel::getStudioGameStateType"]);
    MH_EnableHook(this->m_mapRobloxFunctions["RBX::DataModel::getStudioGameStateType"]);

    this->m_mapHookMap["RBX::DataModel::doDataModelClose"] = new void *();
    MH_CreateHook(this->m_mapRobloxFunctions["RBX::DataModel::doDataModelClose"], rbx__datamodel__dodatamodelclose,
                  &this->m_mapHookMap["RBX::DataModel::doDataModelClose"]);
    MH_EnableHook(this->m_mapRobloxFunctions["RBX::DataModel::doDataModelClose"]);

    this->m_mapHookMap["RBX::RBXCRASH"] = new void *();
    MH_CreateHook(this->m_mapRobloxFunctions["RBX::RBXCRASH"], rbx_rbxcrash, &this->m_mapHookMap["RBX::RBXCRASH"]);
    MH_EnableHook(this->m_mapRobloxFunctions["RBX::RBXCRASH"]);

    logger->PrintInformation(RbxStu::RobloxManager, "Initialization Completed. [3/3]");
    this->m_bInitialized = true;
}

std::shared_ptr<RobloxManager> RobloxManager::GetSingleton() {
    std::lock_guard lock{__robloxmanager__singleton__lock};
    if (RobloxManager::pInstance == nullptr)
        RobloxManager::pInstance = std::make_shared<RobloxManager>();

    if (!RobloxManager::pInstance->m_bInitialized)
        RobloxManager::pInstance->Initialize();


    return RobloxManager::pInstance;
}

std::optional<lua_State *> RobloxManager::GetGlobalState(void *scriptContext) {
    auto logger = Logger::GetSingleton();
    const uint64_t identity = 0;
    const uint64_t script = 0;

    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager,
                           "Failed to Get Global State. Reason: RobloxManager is not initialized.");
        return {};
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::ScriptContext::getGlobalState")) {
        logger->PrintError(RbxStu::RobloxManager, "Failed to Get Global State. Reason: RobloxManager failed to scan "
                                                  "for 'RBX::ScriptContext::getGlobalState'.");
        return {};
    }


    return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ScriptContext_getGlobalState>(
            this->m_mapRobloxFunctions["RBX::ScriptContext::getGlobalState"])(scriptContext, &identity, &script);
}

std::optional<std::int64_t> RobloxManager::IdentityToCapability(const std::int32_t &identity) {
    const auto logger = Logger::GetSingleton();

    if (identity > 10 || identity < 0) {
        logger->PrintError(RbxStu::RobloxManager,
                           "[IdentityToCapability] identity should more than zero and less than or equal to 10");
        return {};
    }

    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager,
                           "Failed to get identity to capability. Reason: RobloxManager is not initialized.");
        return {};
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::Security::IdentityToCapability")) {
        logger->PrintWarning(RbxStu::RobloxManager,
                             "-- WARN: RobloxManager has failed to fetch the address for "
                             "RBX::Security::IdentityToCapability. Falling back to default implementation.");

        // This is a copy of behaviour of the 11/08/2024 (DD/MM/YYYY) for this very function.
        // Roblox does not change this very often, if anything, me implementing the bypass for identity
        // caused them to change it on the client. If this ever breaks, props to Roblox because they're actually reading
        // this useless code comment.
        switch (identity) {
            case 1:
            case 4:
                return 3;
            case 3:
            case 6:
                return 0xB;
            case 5:
                return 1;
            case 7:
            case 8:
                return 0x3F;
            case 9:
                return 0xC;
            case 0xA:
                return 0x4000000000000003;
            default:
                return {};
        }
    }

    return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_Security_IdentityToCapability>(
            this->m_mapRobloxFunctions["RBX::Security::IdentityToCapability"])(&identity);
}

std::optional<RbxStu::StudioFunctionDefinitions::r_RBX_Console_StandardOut> RobloxManager::GetRobloxPrint() {
    auto logger = Logger::GetSingleton();
    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager,
                           "Failed to print to the roblox console. Reason: RobloxManager is not initialized.");
        return {};
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::Console::StandardOut")) {
        logger->PrintWarning(RbxStu::RobloxManager, "-- WARN: RobloxManager has failed to fetch the address for "
                                                    "RBX::Console::StandardOut, printing to console is unavailable.");
        return {};
    }


    return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_Console_StandardOut>(
            this->m_mapRobloxFunctions["RBX::Console::StandardOut"]);
}

std::optional<lua_CFunction> RobloxManager::GetRobloxTaskDefer() {
    auto logger = Logger::GetSingleton();
    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager,
                           "Cannot obtain task_defer. Reason: RobloxManager is not initialized.");
        return {};
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::ScriptContext::task_defer")) {
        logger->PrintWarning(RbxStu::RobloxManager, "-- WARN: RobloxManager has failed to fetch the address for "
                                                    "RBX::ScriptContext::task_defer, deferring a task is unavailable.");
        return {};
    }


    return reinterpret_cast<lua_CFunction>(this->m_mapRobloxFunctions["RBX::ScriptContext::task_defer"]);
}

std::optional<lua_CFunction> RobloxManager::GetRobloxTaskSpawn() {
    const auto logger = Logger::GetSingleton();
    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager,
                           "Cannot obtain task_spawn. Reason: RobloxManager is not initialized.");
        return {};
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::ScriptContext::task_spawn")) {
        logger->PrintWarning(RbxStu::RobloxManager, "-- WARN: RobloxManager has failed to fetch the address for "
                                                    "RBX::ScriptContext::task_spawn, spawning a task is unavailable.");
        return {};
    }


    return reinterpret_cast<lua_CFunction>(this->m_mapRobloxFunctions["RBX::ScriptContext::task_spawn"]);
}
std::optional<void *> RobloxManager::GetScriptContext(lua_State *L) {
    const auto logger = Logger::GetSingleton();
    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager, "Cannot obtain resume. Reason: RobloxManager is not initialized.");
        return {};
    }

    const auto extraSpace = static_cast<RBX::Lua::ExtraSpace *>(L->userdata);
    if (L->userdata == nullptr) {
        logger->PrintWarning(RbxStu::RobloxManager,
                             "Failed to retrieve ScriptContext from lua_State*! L->userdata == nullptr");
        return {};
    }
    const auto scriptContext = extraSpace->sharedExtraSpace->scriptContext;
    return scriptContext;
}

std::optional<RBX::DataModel *> RobloxManager::GetDataModelFromScriptContext(void *scriptContext) {
    const auto logger = Logger::GetSingleton();
    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager, "Cannot obtain resume. Reason: RobloxManager is not initialized.");
        return {};
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::ScriptContext::getDataModel")) {
        logger->PrintWarning(RbxStu::RobloxManager, "-- WARN: RobloxManager has failed to fetch the address for "
                                                    "RBX::ScriptContext::getDataModel, obtaining the DataModel from a "
                                                    "ScriptContext instance is unavailable");
        return {};
    }

    return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ScriptContext_getDataModel>(
            this->m_mapRobloxFunctions["RBX::ScriptContext::getDataModel"])(scriptContext);
}
bool RobloxManager::IsInitialized() const { return this->m_bInitialized; }

void *RobloxManager::GetRobloxFunction(const std::string &functionName) {
    if (this->m_mapRobloxFunctions.contains(functionName)) {
        return this->m_mapRobloxFunctions[functionName];
    }
    return nullptr;
}

void RobloxManager::ResumeScript(RBX::Lua::WeakThreadRef *threadRef, const std::int32_t nret) {
    const auto logger = Logger::GetSingleton();
    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager, "Cannot obtain resume. Reason: RobloxManager is not initialized.");
        return;
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::ScriptContext::resume")) {
        logger->PrintWarning(RbxStu::RobloxManager, "-- WARN: RobloxManager has failed to fetch the address for "
                                                    "RBX::ScriptContext::resume, resuming threads is unavailable");
        return;
    }

    auto resumeFunction = reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ScriptContext_resume>(
            this->m_mapRobloxFunctions["RBX::ScriptContext::resume"]);

    auto extraSpace = static_cast<RBX::Lua::ExtraSpace *>(threadRef->thread->userdata);

    auto scriptContext = extraSpace->sharedExtraSpace->scriptContext;

    const auto out = new int64_t[0x2];
    logger->PrintInformation(RbxStu::RobloxManager,
                             std::format("Resuming thread {}!", reinterpret_cast<void *>(threadRef->thread)));


    /// Roblox has decided to make our lifes more annoying. ScriptContext, when calling resume, must be offset (its
    /// address), by 0x698. This is done as a "Facet Check", ugly stuff, but if we don't do it, we will cause an access
    /// violation, this offset can be updated by searching for xrefs to "[FLog::ScriptContext] Resuming script: %p"

    try {
        resumeFunction(reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(scriptContext) + 0x698), out,
                       &threadRef, nret, false, nullptr);
    } catch (const std::exception &ex) {
        logger->PrintError(RbxStu::RobloxManager,
                           std::format("An error occured whilst resuming the thread! Exception: {}", ex.what()));
    }


    logger->PrintInformation(
            RbxStu::RobloxManager,
            std::format("RBX::ScriptContext::resume : [Status (0x1)]: {}; [Unknown (0x2)]: {}", out[0], out[1]));

    delete[] out;
}

void *RobloxManager::GetHookOriginal(const std::string &functionName) {
    if (this->m_mapHookMap.contains(functionName)) {
        return this->m_mapHookMap[functionName];
    }

    return this->GetRobloxFunction(functionName); // Redirect to GetRobloxFunction.
}

static std::shared_mutex __datamodelModificationMutex;

std::optional<RBX::DataModel *> RobloxManager::GetCurrentDataModel(const RBX::DataModelType &dataModelType) const {
    std::lock_guard lock{__datamodelModificationMutex};
    if (this->m_bInitialized && this->m_mapDataModelMap.contains(dataModelType))
        return this->m_mapDataModelMap.at(dataModelType);

    return {};
}

void RobloxManager::SetCurrentDataModel(const RBX::DataModelType &dataModelType, RBX::DataModel *dataModel) {
    std::lock_guard lock{__datamodelModificationMutex};
    if (this->m_bInitialized) {
        const auto logger = Logger::GetSingleton();
        if (dataModel && dataModel->m_bIsClosed) {
            logger->PrintWarning(RbxStu::RobloxManager,
                                 std::format("Attempted to change the current DataModel of type {} but the provided "
                                             "DataModel is marked as closed!",
                                             RBX::DataModelTypeToString(dataModelType)));
            return;
        }
        if (dataModel == nullptr) {
            logger->PrintError(
                    RbxStu::RobloxManager,
                    std::format("Attempted to change the DataModel pointer from {} to nullptr. This may lead to "
                                "undefined behaviour from callers who expect the DataModel to be a valid pointer! The "
                                "operation will not be allowed to continue.",
                                static_cast<void *>(this->m_mapDataModelMap[dataModelType])));
            return;
        }
        this->m_mapDataModelMap[dataModelType] = dataModel;
        logger->PrintInformation(RbxStu::RobloxManager, std::format("DataModel of type {} modified to point to: {}",
                                                                    RBX::DataModelTypeToString(dataModelType),
                                                                    reinterpret_cast<void *>(dataModel)));
    }
}

bool RobloxManager::IsDataModelValid(const RBX::DataModelType &type) const {
    if (this->m_bInitialized && this->GetCurrentDataModel(type).has_value()) {
        const auto dataModel = this->GetCurrentDataModel(type).value();
        return Utilities::IsPointerValid(dataModel) && !dataModel->m_bIsClosed;
    }

    return false;
}
