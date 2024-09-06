//
// Created by Dottik on 10/8/2024.
//
#pragma once
#include <Windows.h>
#include <hex.h>
#include <sha.h>
#include <sstream>
#include <string>
#include <tlhelp32.h>
#include <vector>

#include "Logger.hpp"
#include "lualib.h"

class Utilities final {
    struct ThreadInformation {
        bool bWasSuspended;
        HANDLE hThread;
    };

    enum ThreadSuspensionState {
        SUSPENDED,
        RESUMED,
    };

public:
    class RobloxThreadSuspension {
        std::vector<ThreadInformation> threadInformation;
        ThreadSuspensionState state;

    public:
        explicit RobloxThreadSuspension(const bool suspendOnCreate) {
            this->state = RESUMED;
            if (suspendOnCreate)
                this->SuspendThreads();
        }

        ~RobloxThreadSuspension() {
            const auto logger = Logger::GetSingleton();
            logger->PrintInformation(RbxStu::ThreadManagement, "Cleaning up thread handles!");
            for (auto &[_, hThread]: this->threadInformation) {
                if (state == SUSPENDED)
                    ResumeThread(hThread);
                CloseHandle(hThread);
            }
        }

        void SuspendThreads() {
            const auto logger = Logger::GetSingleton();
            if (this->state != RESUMED) {
                logger->PrintDebug(RbxStu::ThreadManagement,
                                   "Trying to suspend threads while they are already suspended!");
                return;
            }
            logger->PrintDebug(RbxStu::ThreadManagement, "Pausing roblox threads!");
            const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

            if (hSnapshot == INVALID_HANDLE_VALUE || hSnapshot == nullptr) {
                throw std::exception("PauseRobloxThreads failed: Snapshot creation failed!");
            }

            THREADENTRY32 te{0};
            te.dwSize = sizeof(THREADENTRY32);

            if (!Thread32First(hSnapshot, &te)) {
                CloseHandle(hSnapshot);
                throw std::exception("PauseRobloxThreads failed: Thread32First failed!");
            }
            const auto currentPid = GetCurrentProcessId();
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

            this->threadInformation = thInfo;
            this->state = SUSPENDED;
        }

        void ResumeThreads() {
            const auto logger = Logger::GetSingleton();
            if (this->state != SUSPENDED) {
                logger->PrintDebug(RbxStu::ThreadManagement,
                                   "Attempting to resume threads while they are already resumed!");
                return;
            }
            logger->PrintDebug(RbxStu::ThreadManagement, "Resuming roblox threads!");
            for (auto &[bWasSuspended, hThread]: this->threadInformation) {
                ResumeThread(hThread);
            }
            this->state = RESUMED;
        }
    };

    __forceinline static void checkInstance(lua_State *L, int index, const char *expectedClassname) {
        luaL_checktype(L, index, LUA_TUSERDATA);

        lua_getglobal(L, "typeof");
        lua_pushvalue(L, index);
        lua_call(L, 1, 1);
        const bool isInstance = (strcmp(lua_tostring(L, -1), "Instance") == 0);
        lua_pop(L, 1);

        if (!isInstance)
            luaL_argerror(L, index, "expected an Instance");

        if (strcmp(expectedClassname, "ANY") == 0)
            return;

        lua_getfield(L, index, "IsA");
        lua_pushvalue(L, index);
        lua_pushstring(L, expectedClassname);
        lua_call(L, 2, 1);
        const bool isExpectedClass = lua_toboolean(L, -1);
        lua_pop(L, 1);

        if (!isExpectedClass)
            luaL_argerror(L, index, std::format("Expected to be {}", expectedClassname).c_str());
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

    __forceinline static std::optional<const std::string> GetHwid() {
        auto logger = Logger::GetSingleton();
        HW_PROFILE_INFO hwProfileInfo;
        if (!GetCurrentHwProfileA(&hwProfileInfo)) {
            logger->PrintError(RbxStu::Anonymous, "Failed to obtain Hardware Identifier from GetCurrentHwProfileA, returning empty!");
            return {};
        }

        CryptoPP::SHA256 sha256;
        unsigned char digest[CryptoPP::SHA256::DIGESTSIZE];
        sha256.CalculateDigest(digest, reinterpret_cast<unsigned char *>(hwProfileInfo.szHwProfileGuid),
                               sizeof(hwProfileInfo.szHwProfileGuid));

        CryptoPP::HexEncoder encoder;
        std::string output;
        encoder.Attach(new CryptoPP::StringSink(output));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();

        return output;
    }
};

// Concepts are virtually T : where ... in C#, type constraints on templates.
namespace RbxStu::Concepts {
    template<typename Derived, typename Base>
    concept TypeConstraint = std::is_base_of_v<Base, Derived>;
}
