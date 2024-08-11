//
// Created by Dottik on 10/8/2024.
//

#include <Logger.hpp>
#include <Windows.h>
#include <cstdio>

#include "Scanner.hpp"

int main() {
    AllocConsole();
    const auto logger = Logger::GetSingleton();
    logger->Initialize(true);
    logger->PrintInformation(RbxStu::MainThread, "Hello, world!");

    auto scanner = Scanner::GetSingleton();
    logger->PrintInformation(RbxStu::MainThread, "Launching scanner");
    logger->PrintInformation(RbxStu::MainThread, "Scanning for 80 79 06 00 0F 85 ? ? ? ? E9 ? ? ? ? -- luau_execute");
    auto results = scanner->Scan(SignatureByte::GetSignatureFromIDAString("80 79 06 00 0F 85 ? ? ? ? E9 ? ? ? ?"),
                                 GetModuleHandle(nullptr));

    if (!results.empty()) {
        printf("Found luau_execute candidates!\n");
        printf("luau_execute: %p", *results.data());
    }

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
