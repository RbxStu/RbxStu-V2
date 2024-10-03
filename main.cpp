//
// Created by Dottik on 10/8/2024.
//
#include <Windows.h>

#include <Logger.hpp>
#include <cstdio>
#include <iostream>

#include <DbgHelp.h> // Must be positioned here because else include failure.

#include <ThemidaSDK/ThemidaSDK.h>
#include "Communication/Communication.hpp"
#include "LuauManager.hpp"
#include "RobloxManager.hpp"
#include "Scanner.hpp"
#include "Scheduler.hpp"

#include <obfus.h>

#include "Debugger/DebuggerManager.hpp"
#include "ModLoader/ModManager.hpp"

static void watermark() {
    WATERMARK(R"(/***)", R"( *      ____  _          ____  _          __     ______  )",
              R"( *     |  _ \| |____  __/ ___|| |_ _   _  \ \   / /___ \ )",
              R"( *     | |_) | '_ \ \/ /\___ \| __| | | |  \ \ / /  __) |)",
              R"( *     |  _ <| |_) >  <  ___) | |_| |_| |   \ V /  / __/ )",
              R"( *     |_| \_\_.__/_/\_\|____/ \__|\__,_|    \_/  |_____|)",
              R"( *)"
              R"( */ )");
}

long exception_filter(PEXCEPTION_POINTERS pExceptionPointers) {
    const auto *pContext = pExceptionPointers->ContextRecord;
    printf("\r\n-- WARNING: Exception handler caught an exception\r\n");

    printf("Exception Code: %llx\r\n", pExceptionPointers->ExceptionRecord->ExceptionCode);
    printf("Exception Address: %p\r\n", pExceptionPointers->ExceptionRecord->ExceptionAddress);

    try {
        std::rethrow_exception(std::current_exception());
    } catch (const std::exception &ex) {
        printf("\nIntercepted C++/Cxx exception!\nError Reason: '%s'\n. Resuming SEH handler!\n", ex.what());
    } catch (...) {
        printf("\nNo current C++/Cxx exception? This should never happen but alas, Resuming SEH handler!\n");
    }


    printf("Thread      RI P         @ %p\r\n", reinterpret_cast<void *>(pExceptionPointers->ContextRecord->Rip));
    printf("Module.dll               @ %p\r\n", reinterpret_cast<void *>(GetModuleHandleA("Module.dll")));
    printf("Rebased Module           @ %p\r\n",
           reinterpret_cast<void *>(pExceptionPointers->ContextRecord->Rip -
                                    reinterpret_cast<std::uintptr_t>(GetModuleHandleA("Module.dll"))));
    printf("RobloxStudioBeta.exe     @ %p\r\n", reinterpret_cast<void *>(GetModuleHandleA("RobloxStudioBeta.exe")));

    printf("Rebased Studio           @ %p\r\n",
           reinterpret_cast<void *>(pContext->Rip -
                                    reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxStudioBeta.exe"))));

    printf("-- START REGISTERS STATE --\r\n\r\n");

    printf("-- START GENERAL PURPOSE REGISTERS --\r\n");

    printf("RAX: 0x%llx\r\n", pContext->Rax);
    printf("RBX: 0x%llx\r\n", pContext->Rbx);
    printf("RCX: 0x%llx\r\n", pContext->Rcx);
    printf("RDX: 0x%llx\r\n", pContext->Rdx);
    printf("RDI: 0x%llx\r\n", pContext->Rdi);
    printf("RSI: 0x%llx\r\n", pContext->Rsi);
    printf("R08: 0x%llx\r\n", pContext->R8);
    printf("R09: 0x%llx\r\n", pContext->R9);
    printf("R10: 0x%llx\r\n", pContext->R10);
    printf("R11: 0x%llx\r\n", pContext->R11);
    printf("R12: 0x%llx\r\n", pContext->R12);
    printf("R13: 0x%llx\r\n", pContext->R13);
    printf("R14: 0x%llx\r\n", pContext->R14);
    printf("R15: 0x%llx\r\n", pContext->R15);

    printf("-- END GP REGISTERS --\r\n\r\n");

    printf("-- START STACK POINTERS --\r\n");
    printf("RBP: 0x%llx\r\n", pContext->Rbp);
    printf("RSP: 0x%llx\r\n", pContext->Rsp);
    printf("-- END STACK POINTERS --\r\n\r\n");

    printf("-- END REGISTERS STATE --\r\n\r\n");


    printf("    -- Stack Trace:\r\n");

    const auto hCurrentProcess = GetCurrentProcess();

    SymInitialize(hCurrentProcess, nullptr, true);

    void *stack[256];
    const unsigned short frameCount = RtlCaptureStackBackTrace(0, 255, stack, nullptr);

    for (unsigned short i = 0; i < frameCount; ++i) {
        const auto address = reinterpret_cast<DWORD64>(stack[i]);
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)]; // Stack allocations do not get constantly
                                                                              // reallocated, this ain't the heap.
        auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbolBuffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD value{};
        DWORD *pValue = &value;
        if (SymFromAddr(GetCurrentProcess(), address, nullptr, symbol) && ((*pValue = symbol->Address - address)) &&
            SymFromAddr(GetCurrentProcess(), address, reinterpret_cast<PDWORD64>(pValue), symbol)) {
            printf(("[Stack Frame %d] Inside %s @ %p; Studio Rebase: %p\r\n"), i, symbol->Name,
                   reinterpret_cast<void *>(address),
                   reinterpret_cast<void *>(address -
                                            reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxStudioBeta.exe")) +
                                            0x140000000));
        } else {
            printf(("[Stack Frame %d] Unknown Subroutine @ %p; Studio Rebase: %p\r\n"), i,
                   reinterpret_cast<void *>(address),
                   reinterpret_cast<void *>(address -
                                            reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxStudioBeta.exe")) +
                                            0x140000000));
        }
    }

    // Clean up
    SymCleanup(GetCurrentProcess());
    MessageBoxA(nullptr, ("Studio Crash"), ("RbxStu exception filter has been run! Stacktrace on Studio's CLI."),
                MB_OK);
    printf("Stack frames captured - Waiting for 30s before handing to executee... \r\n");
    Sleep(30000);

    return EXCEPTION_EXECUTE_HANDLER;
}

int main() {
    VM_START;
    STR_ENCRYPT_START;
    watermark();
    SetUnhandledExceptionFilter(exception_filter);
    AllocConsole();
    const auto logger = Logger::GetSingleton();
    logger->Initialize(true);
    printf(R"(//    _____  _          _____ _          __      _____
//   |  __ \| |        / ____| |         \ \    / /__ \
//   | |__) | |____  _| (___ | |_ _   _   \ \  / /   ) |
//   |  _  /| '_ \ \/ /\___ \| __| | | |   \ \/ /   / /
//   | | \ \| |_) >  < ____) | |_| |_| |    \  /   / /_
//   |_|  \_\_.__/_/\_\_____/ \__|\__,_|     \/   |____|
//
)");

    logger->PrintInformation(RbxStu::MainThread, "Initializing RbxStu V2");
    logger->PrintInformation(RbxStu::MainThread,
                             std::format("-- Studio Base: {}", static_cast<void *>(GetModuleHandle(nullptr))));
    logger->PrintInformation(RbxStu::MainThread,
                             std::format("-- RbxStu Base: {}", static_cast<void *>(GetModuleHandle("Module.dll"))));
    SetConsoleTitleA("-- RbxStu V2 --");
    logger->PrintInformation(RbxStu::MainThread, "-- Initializing ModManager...");
    auto modManager = ModManager::GetSingleton();
    modManager->LoadMods();

    logger->PrintInformation(RbxStu::MainThread, "-- Initializing RobloxManager...");
    const auto robloxManager = RobloxManager::GetSingleton();
    logger->PrintInformation(RbxStu::MainThread, "-- Initializing LuauManager...");
    const auto luauManager = LuauManager::GetSingleton();
    logger->PrintInformation(RbxStu::MainThread, "-- Initializing Communication...");

    std::thread(Communication::NewCommunication, "ws://localhost:8523").detach();
    std::thread(Communication::HandlePipe, "CommunicationPipe").detach();

    logger->PrintInformation(RbxStu::MainThread, "-- Initializing DebuggerManager...");
    auto debugManagerInitialize = std::thread([]() {
        const auto debuggerManager = DebuggerManager::GetSingleton();
        debuggerManager->Initialize();
    });

    logger->PrintInformation(RbxStu::MainThread, "Running mod initialization step...");
    modManager->InitializeMods();
    logger->PrintInformation(RbxStu::MainThread, "All mods have been initialized.");

    logger->PrintWarning(RbxStu::MainThread, "Asserting if execution is possible (Evaluating found functions)...");

    try {
        auto hasFailedAnyCheck = false;
        if (robloxManager->GetRobloxFunction("RBX::ScriptContext::task_defer") == nullptr ||
            robloxManager->GetRobloxFunction("RBX::ScriptContext::task_spawn") == nullptr) {

            if (robloxManager->GetRobloxFunction("RBX::ScriptContext::task_defer") == nullptr)
                logger->PrintError(RbxStu::MainThread,
                                   "Failed to find RBX::ScriptContext::task_defer; If RBX::ScriptContext::task_spawn "
                                   "is found, scheduling will not be an issue!");
            if (robloxManager->GetRobloxFunction("RBX::ScriptContext::task_spawn") == nullptr)
                logger->PrintError(RbxStu::MainThread,
                                   "Failed to find RBX::ScriptContext::task_spawn; If RBX::ScriptContext::task_delay "
                                   "is found, scheduling will not be an issue!");

            hasFailedAnyCheck = true;
        } else if (robloxManager->GetRobloxFunction("RBX::ScriptContext::task_defer") == nullptr ||
                   robloxManager->GetRobloxFunction("RBX::ScriptContext::task_spawn") == nullptr) {
            if (robloxManager->GetRobloxFunction("RBX::ScriptContext::task_spawn") == nullptr)
                logger->PrintError(RbxStu::MainThread, "Execution is not possible! Cannot schedule Luau through the "
                                                       "ROBLOX scheduler! Refusing to continue!");

            throw std::exception("RbxStu V2 cannot execute scripts; No method of scheduling through the ROBLOX "
                                 "scheduler was found, AOBs must be updated!");
        }

        if (robloxManager->GetRobloxFunction("RBX::ScriptContext::resumeDelayedThreads") == nullptr) {
            logger->PrintError(RbxStu::MainThread,
                               "Execution is not possible! Cannot safely hook into ROBLOX's scheduler via "
                               "RBX::ScriptContext::resumeDelayedThreads! This makes it impossible to guarantee safe "
                               "environment initialization! Refusing to continue!");

            throw std::exception("RbxStu V2 cannot execute scripts; No way of initializing in a stable manner was "
                                 "found, AOBs must be updated!");
        }

        if (robloxManager->GetRobloxFunction("RBX::ScriptContext::resume") == nullptr) {
            logger->PrintError(RbxStu::MainThread,
                               "Execution, whilst possible, may face issues in some functions! Refusing to continue!");
            throw std::exception("RBX::ScriptContext::resume could not be found! Yielding safely for C closures is not "
                                 "possible, AOBs must be updated!");
        }

        if (robloxManager->GetRobloxFunction("RBX::DataModel::getStudioGameStateType") == nullptr) {
            logger->PrintError(
                    RbxStu::MainThread,
                    "Execution, whilst possible, will be unstable and unsafe as the DataModel that will be used for "
                    "execution may lead to security issues (As it cannot be determined for certain which it is!)");
            throw std::exception(
                    "RBX::DataModel::getStudioGameStateType could not be found; This makes initialization impossible");
        }

        if (robloxManager->GetRobloxFunction("RBX::BasePart::fireTouchSignals") == nullptr) {
            logger->PrintError(RbxStu::MainThread, "firetouchinterest will be unavailable and when error when invoked! "
                                                   "Failed to find the required internal Roblox function!");

            hasFailedAnyCheck = true;
        }

        if (!robloxManager->GetFastVariable("TaskSchedulerTargetFps").has_value()) {
            logger->PrintError(RbxStu::MainThread, "setfpscap and getfpscap are unavailable and will error when "
                                                   "invoked! Failed to find required IntFastFlag!");
            hasFailedAnyCheck = true;
        }

        if (hasFailedAnyCheck) {
            logger->PrintWarning(RbxStu::MainThread, "RbxStu V2 may need to update its AOBs, some offset is missing, "
                                                     "but it should not have any major issues on execution!");
        } else {
            logger->PrintWarning(
                    RbxStu::MainThread,
                    "RbxStu V2 has all the required offsets to work as expected, no further action is required.");
        }
    } catch (const std::exception &ex) {
        logger->PrintError(RbxStu::MainThread,
                           "RbxStu V2 NEEDS to update! A significant offset is missing, which may hinder execution in "
                           "a significant manner! Please contact the developers, RbxStu V2 will now crash :'(");
        throw;
    }

    logger->PrintInformation(RbxStu::MainThread, "Waiting for DebuggerManager...");
    debugManagerInitialize.join();

    logger->PrintInformation(RbxStu::MainThread,
                             "Main Thread will now close, as RbxStu V2's initialization has been completed.");

    STR_ENCRYPT_END;
    VM_END;
    return 0;
}


BOOL WINAPI DllMain(const HINSTANCE hModule, const DWORD fdwReason, const LPVOID lpvReserved) {
    // Perform actions based on the reason for calling.
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            CreateThread(nullptr, 0x1000, reinterpret_cast<LPTHREAD_START_ROUTINE>(main), nullptr, 0, nullptr);
            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:

            if (lpvReserved != nullptr) {
                break; // do not do cleanup if process termination scenario
            }

            // Perform any necessary cleanup.
            break;
        default:

            break;
    }
    return true;
}
