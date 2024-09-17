//
// Created by Dottik on 15/9/2024.
//

#pragma once
#include "Environment/EnvironmentManager.hpp"
#include "Roblox/TypeDefinitions.hpp"


class Debugger final : public Library {
public:
    std::string GetLibraryName() override;
    luaL_Reg *GetLibraryFunctions() override;
};
