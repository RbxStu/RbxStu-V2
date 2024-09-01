//
// Created by Dottik on 1/9/2024.
//

#pragma once
#include "Environment/EnvironmentManager.hpp"


class Http final : public Library {
public:
    std::string GetLibraryName();
    luaL_Reg *GetLibraryFunctions();
};

