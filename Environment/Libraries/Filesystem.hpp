//
// Created by Pixeluted on 20/08/2024.
//

#pragma once
#include "Environment/EnvironmentManager.hpp"

class Filesystem final : public Library {
public:
    std::string GetLibraryName() override;
    luaL_Reg *GetLibraryFunctions() override;
};
