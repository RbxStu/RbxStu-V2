//
// Created by Dottik on 10/8/2024.
//
#include <Windows.h>

#include <Logger.hpp>
#include <cstdio>
#include <iostream>

#include <DbgHelp.h> // Must be positioned here because else include failure.

#include "Communication/Communication.hpp"
#include "LuauManager.hpp"
#include "RobloxManager.hpp"
#include "Scanner.hpp"
#include "Scheduler.hpp"

long exception_filter(PEXCEPTION_POINTERS pExceptionPointers) {
    const auto *pContext = pExceptionPointers->ContextRecord;
    printf("\r\n-- WARNING: Exception handler caught an exception\r\n");

    if (pExceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        printf("Exception Identified: EXCEPTION_ACCESS_VIOLATION\r\n");
    }
    try {
        std::rethrow_exception(std::current_exception());
    } catch (const std::exception &ex) {
        printf("\nIntercepted C++/Cxx exception!\nError Reason: '%s'\n. Resuming SEH handler!\n", ex.what());
    } catch (...) {
        printf("\nNo current C++/Cxx exception? Resuming SEH handler!\n");
    }


    printf("Exception Caught         @ %p\r\n", pExceptionPointers->ContextRecord->Rip);
    printf("Module.dll               @ %p\r\n", reinterpret_cast<std::uintptr_t>(GetModuleHandleA("Module.dll")));
    printf("Rebased Module           @ 0x%p\r\n",
           pExceptionPointers->ContextRecord->Rip - reinterpret_cast<std::uintptr_t>(GetModuleHandleA("Module.dll")));
    printf("RobloxStudioBeta.exe     @ %p\r\n",
           reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxStudioBeta.exe")));

    printf("Rebased Studio           @ 0x%p\r\n",
           pContext->Rip - reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxStudioBeta.exe")));

    printf("-- START REGISTERS STATE --\r\n\r\n");

    printf("-- START GP REGISTERS --\r\n");

    printf("RAX: 0x%p\r\n", pContext->Rax);
    printf("RBX: 0x%p\r\n", pContext->Rbx);
    printf("RCX: 0x%p\r\n", pContext->Rcx);
    printf("RDX: 0x%p\r\n", pContext->Rdx);
    printf("RDI: 0x%p\r\n", pContext->Rdi);
    printf("RSI: 0x%p\r\n", pContext->Rsi);
    printf("-- R8 - R15 --\r\n");
    printf("R08: 0x%p\r\n", pContext->R8);
    printf("R09: 0x%p\r\n", pContext->R9);
    printf("R10: 0x%p\r\n", pContext->R10);
    printf("R11: 0x%p\r\n", pContext->R11);
    printf("R12: 0x%p\r\n", pContext->R12);
    printf("R13: 0x%p\r\n", pContext->R13);
    printf("R14: 0x%p\r\n", pContext->R14);
    printf("R15: 0x%p\r\n", pContext->R15);
    printf("-- END GP REGISTERS --\r\n\r\n");

    printf("-- START STACK POINTERS --\r\n");
    printf("RBP: 0x%p\r\n", pContext->Rbp);
    printf("RSP: 0x%p\r\n", pContext->Rsp);
    printf("-- END STACK POINTERS --\r\n\r\n");

    printf("-- END REGISTERS STATE --\r\n\r\n");


    printf("    -- Stack Trace:\r\n");
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
    std::cout << std::endl;
    std::stringstream sstream{};
    for (unsigned short i = 0; i < frameCount; ++i) {
        sstream << ("0x") << std::hex << reinterpret_cast<std::uintptr_t>(stack[i]);
        if (i < frameCount) {
            sstream << (" -> ");
        } else {
            sstream << ("\r\n");
        }
    }

    std::cout << sstream.str();

    // Clean up
    SymCleanup(GetCurrentProcess());
    MessageBoxA(nullptr, ("Studio Crash"), ("RbxStu exception filter has been run! Stacktrace on Studio's CLI."),
                MB_OK);
    printf("Stack frames captured - Waiting for 30s before handing to executee... \r\n");
    Sleep(30000);

    return EXCEPTION_EXECUTE_HANDLER;
}

int main() {
    SetUnhandledExceptionFilter(exception_filter);
    AllocConsole();
    const auto logger = Logger::GetSingleton();
    logger->Initialize(true);
    logger->PrintInformation(RbxStu::MainThread,
                             std::format("-- Studio Base: {}", static_cast<void *>(GetModuleHandle(nullptr))));
    logger->PrintInformation(RbxStu::MainThread,
                             std::format("-- RbxStu Base: {}", static_cast<void *>(GetModuleHandle("Module.dll"))));
    logger->PrintInformation(RbxStu::MainThread, "Initializing RbxStu V2");
    SetConsoleTitleA("-- RbxStu V2 --");
    logger->PrintInformation(RbxStu::MainThread, "-- Initializing RobloxManager...");
    const auto robloxManager = RobloxManager::GetSingleton();
    logger->PrintInformation(RbxStu::MainThread, "-- Initializing LuauManager...");
    const auto luauManager = LuauManager::GetSingleton();
    logger->PrintInformation(RbxStu::MainThread, "-- Initializing Communication...");

    std::thread(Communication::NewCommunication, "ws://localhost:8523").detach();
    std::thread(Communication::HandlePipe, "CommunicationPipe").detach();

    const auto robloxPrint = robloxManager->GetRobloxPrint().value();

    logger->PrintInformation(RbxStu::MainThread,
                             "Main Thread will now close, as all initialization has been completed.");

    const auto scheduler = Scheduler::GetSingleton();
    while (true) {
        if (!robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
            _mm_pause();
            continue;
        }

        robloxPrint(RBX::Console::MessageType::InformationBlue, "RbxStu: Client DataModel obtained!");
        logger->PrintInformation(RbxStu::MainThread, "Obtained Client DataModel");

        while (robloxManager->IsDataModelValid(RBX::DataModelType_PlayClient)) {
            _mm_pause();
        }

        robloxPrint(RBX::Console::MessageType::Warning,
                    "RbxStu: Client DataModel lost. Awaiting for new Client DataModel...");
        logger->PrintInformation(RbxStu::MainThread, "Client DataModel lost. Awaiting for new Client DataModel...");
    }

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
            ;
    }
    return true;
}
