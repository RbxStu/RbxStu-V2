//
// Created by Dottik on 10/8/2024.
//

#include <Logger.hpp>
#include <Windows.h>
#include <cstdio>

int main() {
    AllocConsole();
    const auto logger = Logger::GetSingleton();
    logger->Initialize();
logger->PrintInformation(RbxStu::MainThread, "Hello, world!");

    Sleep(1000);
    return 0;
}

BOOL WINAPI DllMain(const HINSTANCE hModule, const DWORD fdwReason, const LPVOID lpvReserved) {
    if (hModule == nullptr || hModule == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "This method of injection is not supported; Please use LoadLibary to load the RbxStu module.");
        abort();
    }
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
