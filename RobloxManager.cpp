//
// Created by Dottik on 11/8/2024.
//

#include "RobloxManager.hpp"

#include <MinHook.h>
#include <shared_mutex>

#include "Scanner.hpp"

std::shared_mutex __robloxmanager__singleton__lock;

void *rbx__scriptcontext__resumeWaitingThreads(void *scriptContext) {
    auto robloxManager = RobloxManager::GetSingleton();
    // auto logger = Logger::GetSingleton();

    return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_ScriptContext_resumeDelayedThreads>(
            robloxManager->GetHookOriginal("RBX::ScriptContext::resumeDelayedThreads"))(scriptContext);
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
        robloxManager->SetCurrentDataModel(getStudioGameStateType(dataModel), nullptr);
    }


    return original(dataModelContainer);
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
            this->m_mapRobloxFunctions["RBX::ScriptContext::getGlobalState"])(scriptContext, nullptr, nullptr);
}

std::optional<std::int64_t> RobloxManager::IdentityToCapability(std::int32_t identity) {
    auto logger = Logger::GetSingleton();
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
RbxStu::StudioFunctionDefinitions::r_RBX_Console_StandardOut RobloxManager::GetRobloxPrint() {
    auto logger = Logger::GetSingleton();
    if (!this->m_bInitialized) {
        logger->PrintError(RbxStu::RobloxManager,
                           "Failed to print to the roblox console. Reason: RobloxManager is not initialized.");
        return nullptr;
    }

    if (!this->m_mapRobloxFunctions.contains("RBX::Console::StandardOut")) {
        logger->PrintWarning(RbxStu::RobloxManager, "-- WARN: RobloxManager has failed to fetch the address for "
                                                    "RBX::Console::StandardOut, printing to console is unavailable.");
        return nullptr;
    }


    return reinterpret_cast<RbxStu::StudioFunctionDefinitions::r_RBX_Console_StandardOut>(
            this->m_mapRobloxFunctions["RBX::Console::StandardOut"]);
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

    return nullptr;
}

static std::shared_mutex __datamodelModificationMutex;

RBX::DataModel *RobloxManager::GetCurrentDataModel(RBX::DataModelType dataModelType) const {
    std::lock_guard lock{__datamodelModificationMutex};
    if (this->m_bInitialized && this->m_mapDataModelMap.contains(dataModelType))
        return this->m_mapDataModelMap.at(dataModelType);

    return nullptr;
}

void RobloxManager::SetCurrentDataModel(RBX::DataModelType dataModelType, RBX::DataModel *dataModel) {
    std::lock_guard lock{__datamodelModificationMutex};
    if (this->m_bInitialized && dataModel != nullptr) {
        const auto logger = Logger::GetSingleton();
        if (dataModel->m_bIsClosed) {
            logger->PrintWarning(RbxStu::RobloxManager,
                                 std::format("Attempted to change the current DataModel of type {} but the provided "
                                             "DataModel is marked as closed!",
                                             RBX::DataModelTypeToString(dataModelType)));
            return;
        }
        this->m_mapDataModelMap[dataModelType] = dataModel;
        logger->PrintInformation(RbxStu::RobloxManager, std::format("DataModel of type {} modified to point to: {}",
                                                                    RBX::DataModelTypeToString(dataModelType),
                                                                    reinterpret_cast<void *>(dataModel)));
    }
}

bool RobloxManager::IsDataModelValid(const RBX::DataModelType &type) const {
    if (this->m_bInitialized && this->GetCurrentDataModel(type)) {
        return Utilities::IsPointerValid(this->GetCurrentDataModel(type)) &&
               !this->GetCurrentDataModel(type)->m_bIsClosed;
    }

    return false;
}
