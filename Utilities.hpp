//
// Created by Dottik on 10/8/2024.
//
#pragma once
#include <Windows.h>
#include <sstream>
#include <string>
#include <vector>

class Utilities final {
public:
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

    __forceinline static bool IsWine() {
        return GetProcAddress(GetModuleHandle("ntdll.dll"), "wine_get_version") != nullptr;
    }

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
