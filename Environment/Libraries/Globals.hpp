//
// Created by Dottik on 18/8/2024.
//
#pragma once
#include "Environment/EnvironmentManager.hpp"


class Globals final : public Library {
public:
    [[nodiscard]] std::string GetLibraryName() const override;
    [[nodiscard]] std::int32_t GetFunctionCount() const override;
    [[nodiscard]] luaL_Reg *GetLibraryFunctions() const override;
};
