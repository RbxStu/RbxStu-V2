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
    __inline static std::vector<std::string> SplitBy(const std::string &target, const char split) {
        std::vector<std::string> splitted;
        std::stringstream stream(target);
        std::string temporal;
        while (std::getline(stream, temporal, split)) {
            splitted.push_back(temporal);
            temporal.clear();
        }

        return splitted;
    }

    __forceinline bool IsWine() { return GetProcAddress(GetModuleHandle("ntdll.dll"), "wine_get_version") != nullptr; }
};
