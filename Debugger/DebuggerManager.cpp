//
// Created by Dottik on 15/9/2024.
//

#include "DebuggerManager.hpp"

#include <MinHook.h>

#include "Communication/PacketSerdes.hpp"
#include "Disassembler/Disassembler.hpp"
#include "Environment/Libraries/Debugger.hpp"
#include "Luau/Compiler.h"
#include "LuauManager.hpp"
#include "RobloxManager.hpp"
#include "Scanner.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "ldebug.h"

std::shared_ptr<DebuggerManager> DebuggerManager::pInstance;

std::mutex __DebugManagerInitializationMutex;

RbxStu::LuauFunctionDefinitions::luau_load original_rluau_load;
RBX::Studio::FunctionTypes::luau_execute original_rluau_execute;

void rluau_execute__detour(lua_State *L) {
    const auto debugManager = DebuggerManager::GetSingleton();
    const auto currentCl = clvalue(L->ci->func);
    if (!currentCl->isC) {

        if (debugManager->IsLocalPlayerScript(currentCl->l.p->source->data)) {
            printf("LocalScript Found: %s\n", currentCl->l.p->source->data);
        }

        if (debugManager->IsServerScript(currentCl->l.p->source->data)) {
            printf("Server Script Found: %s\n", currentCl->l.p->source->data);
        }
    }

    return original_rluau_execute(L);
}

lua_Status rluau_load__detour(lua_State *L, const char *chunkname, const char *data, size_t size, int env) {
    const auto debugManager = DebuggerManager::GetSingleton();
    debugManager->PushScriptTracking(chunkname, L);
    return original_rluau_load(L, chunkname, data, size, env);
}

void DebuggerManager::Initialize() {
    std::scoped_lock lg{__DebugManagerInitializationMutex};
    const auto logger = Logger::GetSingleton();
    if (this->m_bIsInitialized) {
        logger->PrintWarning(RbxStu::DebuggerManager, "This instance is already initialized!");
        return;
    }
    this->m_bIsInitialized = true;

    const auto luauManager = LuauManager::GetSingleton();
    const auto scanner = Scanner::GetSingleton();
    logger->PrintInformation(RbxStu::DebuggerManager, "Initializing Debugger Manager [1/3]");
    logger->PrintInformation(RbxStu::DebuggerManager, "Initializing MinHook for function hooking [1/3]");
    MH_Initialize();

    logger->PrintInformation(RbxStu::DebuggerManager, "Obtaining RbxStu::Disassembler for disassembling [1/3]");
    const auto disassembler = Disassembler::GetSingleton();

    const auto robloxManager = RobloxManager::GetSingleton();
    logger->PrintWarning(
            RbxStu::DebuggerManager,
            "Waiting for RBX::ScriptContext::resumeWaitingScripts hook to provide lua_Callbacks *... [2/3]");

    do {
        _mm_pause();
    } while (!this->m_bObtainedCallbacks);

    logger->PrintInformation(RbxStu::DebuggerManager, "Callbacks obtained! [3/3]");

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    logger->PrintInformation(RbxStu::DebuggerManager, "Functions Found via lua_State* scraping:");
    for (const auto &[funcName, funcAddress]: this->m_mapDebugManagerFunctions) {
        logger->PrintInformation(RbxStu::DebuggerManager, std::format("- '{}' at address {}.", funcName, funcAddress));
    }

    logger->PrintInformation(RbxStu::DebuggerManager, "Setting up hook on luau_load to track bytecode load-ins...");

    const auto rLuauLoad =
            reinterpret_cast<RbxStu::LuauFunctionDefinitions::luau_load>(luauManager->GetFunction("luau_load"));

    MH_CreateHook(rLuauLoad, rluau_load__detour, reinterpret_cast<void **>(&original_rluau_load));
    MH_EnableHook(rLuauLoad);

    logger->PrintInformation(RbxStu::DebuggerManager, "Set hook. luau_load -> Instrument bytecode loading");

    logger->PrintInformation(RbxStu::DebuggerManager, "Setting up hook on luau_execute to track execution.");

    const auto rLuauExecute =
            reinterpret_cast<RBX::Studio::FunctionTypes::luau_execute>(luauManager->GetFunction("luau_execute"));

    MH_CreateHook(rLuauExecute, rluau_execute__detour, reinterpret_cast<void **>(&original_rluau_execute));
    MH_EnableHook(rLuauExecute);

    logger->PrintInformation(RbxStu::DebuggerManager, "Set hook. luau_execute -> Instrument script execution.");
}
void DebuggerManager::PushScriptTracking(const char *chunkname, lua_State *scriptState) {

    if (!Utilities::IsPointerValid(chunkname) || !Utilities::IsPointerValid(scriptState))
        return;

    this->m_mapScriptStateMap[chunkname] = scriptState;
}

std::shared_ptr<DebuggerManager> DebuggerManager::GetSingleton() {
    if (DebuggerManager::pInstance == nullptr)
        DebuggerManager::pInstance = std::make_shared<DebuggerManager>();

    return DebuggerManager::pInstance;
}
bool DebuggerManager::IsInitialized() const { return this->m_bIsInitialized && this->m_bObtainedCallbacks; }

void DebuggerManager::RegisterCallbackCopy(const lua_Callbacks *callbacks) {
    if (this->m_bObtainedCallbacks)
        return;
    const auto logger = Logger::GetSingleton();

    if (callbacks->debuginterrupt == nullptr || callbacks->debugstep == nullptr || callbacks->debugbreak == nullptr ||
        callbacks->debugprotectederror == nullptr)
        return;

    logger->PrintInformation(RbxStu::DebuggerManager, "Registering a callback from copy...");

    m_mapDebugManagerFunctions["RBX::Scripting::DebuggerManager::hookInterrupt"] = callbacks->debuginterrupt;
    m_mapDebugManagerFunctions["RBX::Scripting::DebuggerManager::hookStep"] = callbacks->debugstep;
    m_mapDebugManagerFunctions["RBX::Scripting::DebuggerManager::hookBreak"] = callbacks->debugbreak;
    m_mapDebugManagerFunctions["RBX::Scripting::DebuggerManager::hookDebugProtectedError"] =
            callbacks->debugprotectederror;
    this->m_bObtainedCallbacks = true;
}
bool DebuggerManager::IsLocalPlayerScript(const std::string_view &chunkName) const {
    return chunkName.find("=Workspace") != std::string::npos || chunkName.find("=Player") != std::string::npos;
}
bool DebuggerManager::IsServerScript(const std::string_view &chunkName) const {
    return chunkName.find("=ServerScriptService") != std::string::npos;
}

bool DebuggerManager::IsBreakpointOnEntry(const std::string &chunkName) const {
    if (!this->m_bIsInitialized || !this->m_mapDebuggerInformationMap.contains(chunkName))
        return false;

    const auto x = this->m_mapDebuggerInformationMap.at(chunkName);
    return x->bBreakpointOnEntry;
}
