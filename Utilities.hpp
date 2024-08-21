//
// Created by Dottik on 10/8/2024.
//
#pragma once
#include <Windows.h>
#include <sstream>
#include <string>
#include <tlhelp32.h>
#include <vector>

#include "Logger.hpp"

class Utilities final {
    struct ThreadInformation {
        bool bWasSuspended;
        HANDLE hThread;
    };

public:
    __forceinline static std::vector<ThreadInformation> PauseTheWorld() {
        const auto logger = Logger::GetSingleton();
        logger->PrintInformation(RbxStu::ThreadManagement, "Pausing the world!");
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

        if (hSnapshot == INVALID_HANDLE_VALUE || hSnapshot == nullptr) {
            throw std::exception("PauseTheWorld failed: Snapshot creation failed!");
        }

        THREADENTRY32 te{0};
        te.dwSize = sizeof(THREADENTRY32);

        if (!Thread32First(hSnapshot, &te)) {
            CloseHandle(hSnapshot);
            throw std::exception("PauseTheWorld failed: Thread32First failed!");
        }
        auto currentPid = GetCurrentProcessId();
        std::vector<ThreadInformation> thInfo;
        do {
            if (te.th32ThreadID != GetCurrentThreadId() && te.th32OwnerProcessID == currentPid) {
                auto hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);

                auto th = ThreadInformation{false, nullptr};
                if (SuspendThread(hThread) > 1)
                    th.bWasSuspended = true;
                th.hThread = hThread;
                thInfo.push_back(th);
            }
        } while (Thread32Next(hSnapshot, &te));

        return thInfo;
    }

    __forceinline static void ResumeWorld(const std::vector<ThreadInformation> &worldInfo) {
        const auto logger = Logger::GetSingleton();
        logger->PrintInformation(RbxStu::ThreadManagement, "Resuming the world!");
        for (auto &[bWasSuspended, hThread]: worldInfo) {
            ResumeThread(hThread);
        }
    }

    __forceinline static void CleanUpWorldInformation(const std::vector<ThreadInformation> &worldInfo) {
        const auto logger = Logger::GetSingleton();
        logger->PrintInformation(RbxStu::ThreadManagement, "Closing thread handles");
        for (auto &[_, hThread]: worldInfo) {
            CloseHandle(hThread);
        }
    }

    __forceinline static std::string ToLower(std::string target) {
        for (auto &x: target) {
            x = std::tolower(x); // NOLINT(*-narrowing-conversions)
        }

        return target;
    }

    /// @brief Splits the given std::string into a std::vector<std::string> using the given character as a
    /// separator.
    __forceinline static std::vector<std::string> SplitBy(const std::string &target, const char split) {
        std::vector<std::string> splitted;
        std::stringstream stream(target);
        std::string temporal;
        while (std::getline(stream, temporal, split)) {
            splitted.push_back(temporal);
            temporal.clear();
        }

        return splitted;
    }

    /// @return True if the DLL is running in a WINE powered environment underneath Linux.
    __forceinline static bool IsWine() {
        return GetProcAddress(GetModuleHandle("ntdll.dll"), "wine_get_version") != nullptr;
    }

    /// @brief Used to validate a pointer.
    /// @remarks This template does NOT validate ANY data inside the pointer. It just validates that the pointer is
    /// at LEAST of the size of the given type, and that the pointer is allocated in memory.
    template<typename T>
    __forceinline static bool IsPointerValid(T *tValue) { // Validate pointers.
        const auto ptr = reinterpret_cast<void *>(tValue);
        auto buf = MEMORY_BASIC_INFORMATION{};

        // Query a full page.
        if (const auto read = VirtualQuery(ptr, &buf, sizeof(buf)); read != 0 && sizeof(buf) != read) {
            // I honestly dont care.
        } else if (read == 0) {
            return false;
        }

        if (buf.RegionSize < sizeof(T)) {
            return false; // Allocated region is too small to fit type T inside.
        }

        if (buf.State & MEM_FREE == MEM_FREE) {
            return false; // The memory is not owned by the process, no need to do anything, we can already assume
            // we cannot read it.
        }

        auto validProtections = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                                PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

        if (buf.Protect & validProtections) {
            return true;
        }
        if (buf.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
            return false;
        }

        return true;
    }
};
