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

    results = scanner->Scan(RbxStu::Signatures::RBX_ExtraSpace_initialize);

    if (results.empty()) {
        logger->PrintWarning(RbxStu::RobloxManager, "Failed to find function 'RBX::ExtraSpace::initialize'!");
    } else {
        this->m_mapRobloxFunctions["RBX::ExtraSpace::initialize"] = *results.data();
    }

    logger->PrintInformation(RbxStu::RobloxManager, "Functions Found via scanning:");
    for (const auto &[funcName, funcAddress]: this->m_mapRobloxFunctions) {
        logger->PrintInformation(RbxStu::RobloxManager, std::format("- '{}' at address 0x{}.", funcName, funcAddress));
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
