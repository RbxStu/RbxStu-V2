//
// Created by Dottik on 18/8/2024.
//
#pragma once
#include "Environment/EnvironmentManager.hpp"
#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu {
    static std::map<RBX::DataModelType, std::map<const void *, int>> s_mRefsMapBasedOnDataModel;
}

class Globals final : public Library {
public:
    std::string GetLibraryName() override;
    luaL_Reg *GetLibraryFunctions() override;
};
