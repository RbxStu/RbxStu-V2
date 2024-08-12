//
// Created by Dottik on 11/8/2024.
//

#include "RobloxManager.hpp"
#include "Scanner.hpp"


std::shared_ptr<RobloxManager> RobloxManager::pInstance;

void RobloxManager::Initialize() {
    auto logger = Logger::GetSingleton();
    if (this->m_bInitialized) {
        logger->PrintWarning(RbxStu::RobloxManager, "This instance is already initialized!");
        return;
    }
    auto scanner = Scanner::GetSingleton();
    logger->PrintInformation(RbxStu::RobloxManager, "Initializing Roblox Manager [1/3]");
    logger->PrintInformation(RbxStu::RobloxManager, "Scanning for functions... [1/3]");

    std::vector<void *> results{};

    results = scanner->Scan(RbxStu::Signatures::RBX_ScriptContext_scriptStart);

    if (results.empty()) {
        logger->PrintWarning(RbxStu::RobloxManager, "Failed to find function 'RBX::ScriptContext::scriptStart'!");
    } else {
        this->m_mapRobloxFunctions["RBX::ScriptContext::scriptStart"] =
                *results.data(); // Grab first result, it doesn't really matter to be honest.
    }

    results = scanner->Scan(RbxStu::Signatures::RBX_ScriptContext_openStateImpl);

    if (results.empty()) {
        logger->PrintWarning(RbxStu::RobloxManager, "Failed to find function 'RBX::ScriptContext::openStateImpl'!");
    } else {
        this->m_mapRobloxFunctions["RBX::ScriptContext::openStateImpl"] = *results.data();
    }

    results = scanner->Scan(RbxStu::Signatures::RBX_ExtraSpace_initializeFrom);

    if (results.empty()) {
        logger->PrintWarning(RbxStu::RobloxManager, "Failed to find function 'RBX::ExtraSpace::initializeFrom'!");
    } else {
        this->m_mapRobloxFunctions["RBX::ExtraSpace::initializeFrom"] = *results.data();
    }

    results = scanner->Scan(RbxStu::Signatures::RBX_ScriptContext_GetGlobalState);

    if (results.empty()) {
        logger->PrintWarning(RbxStu::RobloxManager, "Failed to find function 'RBX::ScriptContext::getGlobalState'!");
    } else {
        this->m_mapRobloxFunctions["RBX::ScriptContext::getGlobalState"] = *results.data();
    }

    results = scanner->Scan(RbxStu::Signatures::RBX_Security_IdentityToCapability);

    if (results.empty()) {
        logger->PrintWarning(RbxStu::RobloxManager, "Failed to find function 'RBX::Security::IdentityToCapability'!");
    } else {
        this->m_mapRobloxFunctions["RBX::Security::IdentityToCapability"] = *results.data();
    }

    results = scanner->Scan(RbxStu::Signatures::RBX_ProximityPrompt_onTriggered);

    if (results.empty()) {
        logger->PrintWarning(RbxStu::RobloxManager, "Failed to find function 'RBX::ProximityPrompt::onTriggered'!");
    } else {
        this->m_mapRobloxFunctions["RBX::ProximityPrompt::onTriggered"] = *results.data();
    }


    logger->PrintInformation(RbxStu::RobloxManager, "Functions Found via scanning:");
    for (const auto &[funcName, funcAddress]: this->m_mapRobloxFunctions) {
        logger->PrintInformation(RbxStu::RobloxManager, std::format("- '{}' at address {}.", funcName, funcAddress));
    }


    logger->PrintInformation(RbxStu::RobloxManager, "Initializing hooks... [2/3]");

    logger->PrintInformation(RbxStu::RobloxManager, "Initialization Completed. [3/3]");
    this->m_bInitialized = true;
}
std::shared_ptr<RobloxManager> RobloxManager::GetSingleton() {
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


    return reinterpret_cast<RbxStu::FunctionDefinitions::r_RBX_ScriptContext_getGlobalState>(
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
                             "-- WARN: RobloxManager has failed to fetch the appropiate signature for "
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

    return reinterpret_cast<RbxStu::FunctionDefinitions::r_RBX_Security_IdentityToCapability>(
            this->m_mapRobloxFunctions["RBX::Security::IdentityToCapability"])(&identity);
}
