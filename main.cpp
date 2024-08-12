//
// Created by Dottik on 10/8/2024.
//

#include <Logger.hpp>
#include <Windows.h>
#include <cstdio>

#include "RobloxManager.hpp"
#include "Scanner.hpp"

int main() {
    AllocConsole();
    const auto logger = Logger::GetSingleton();
    logger->Initialize(true);
    logger->PrintInformation(RbxStu::MainThread, "Initializing RbxStu V2");
    logger->PrintInformation(RbxStu::MainThread, "-- Initializing RobloxManager...");
    const auto robloxManager = RobloxManager::GetSingleton();

    return 0;
}

BOOL WINAPI DllMain(const HINSTANCE hModule, const DWORD fdwReason, const LPVOID lpvReserved) {
    // Perform actions based on the reason for calling.
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
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
    }
    return true; // Successful DLL_PROCESS_ATTACH.
}
