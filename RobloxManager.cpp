//
// Created by Dottik on 11/8/2024.
//

#include "RobloxManager.hpp"

#include <MinHook.h>
#include <shared_mutex>

#include "Scanner.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"

std::shared_mutex __robloxmanager__singleton__lock;

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
    auto ScriptContext =
            *reinterpret_cast<void **>(*reinterpret_cast<std::uintptr_t *>(waitingHybridScriptsJob) + 0x1F8);

    // logger->PrintInformation(RbxStu::HookedFunction,
    //                         std::format("ScriptContext::resumeWaitingThreads. ScriptContext: {:#x}", ScriptContext));

    if (!scheduler->IsInitialized()) { // !scheduler->is_initialized()
        auto getDataModel = reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_DataModel_getDataModel>(
                robloxManager->GetRobloxFunction("RBX::ScriptContext::getDataModel"));
        if (getDataModel == nullptr) {
            logger->PrintWarning(RbxStu::HookedFunction, "Initialization of Scheduler may be unstable! Cannot "
                                                         "determine DataModel for the obtained ScriptContext!");
        } else {
            const auto expectedDataModel = robloxManager->GetCurrentDataModel(RBX::DataModelType_PlayClient);
            if (!expectedDataModel.has_value() || getDataModel(ScriptContext) != expectedDataModel.value() ||
                !robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
                goto __scriptContext_resumeWaitingThreads__cleanup;
            }
        }

        // HACK!: We do not want to initialize the scheduler on the
        // first resumptions of waiting threads. This will cause
        // us to access invalid memory, as the global state is not truly set up yet apparently,
        // race conditions at their finest!
        if (calledBeforeCount <= 6) {
            calledBeforeCount += 1;
            goto __scriptContext_resumeWaitingThreads__cleanup;
        }

        const auto rL = robloxManager->GetGlobalState(ScriptContext);
        if (rL.has_value() && robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
            const auto robloxL = rL.value();
            lua_State *L = lua_newthread(robloxL);
            security->SetThreadSecurity(L);

            const auto dataModel = robloxManager->GetCurrentDataModel(RBX::DataModelType_PlayClient);

            if (!dataModel.has_value() && robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
                logger->PrintError(RbxStu::HookedFunction, "DataModel has become invalid on scheduler initialization! "
                                                           "Assuming the play test has stopped!");
                goto __scriptContext_resumeWaitingThreads__cleanup;
            }
            scheduler->InitializeWith(L, robloxL, dataModel.value());
            lua_pop(L, 1); // Reset stack.
        }
    } else if (robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient) && scheduler->IsInitialized()) {
        scheduler->StepScheduler(scheduler->GetGlobalExecutorState().value());
    } else {
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

    // logger->PrintInformation(RbxStu::RobloxManager, "Scanning for functions (Specialized step)... [2/3]");

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
    const uint64_t identity = 8;
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
    auto logger = Logger::GetSingleton();
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

bool RobloxManager::IsInitialized() const { return this->m_bInitialized; }

void *RobloxManager::GetRobloxFunction(const std::string &functionName) {
    if (this->m_mapRobloxFunctions.contains(functionName)) {
        return this->m_mapRobloxFunctions[functionName];
    }
    return nullptr;
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
