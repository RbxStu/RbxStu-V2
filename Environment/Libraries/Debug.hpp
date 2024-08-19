//
// Created by Dottik on 19/8/2024.
//

#pragma once
#include <lua.h>
#include <string>
#include "Environment/EnvironmentManager.hpp"

class Debug final : public Library {
public:
    std::string GetLibraryName() override;
    luaL_Reg *GetLibraryFunctions() override;
};
