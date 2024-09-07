//
// Created by Dottik on 18/8/2024.
//
#pragma once
#include "Environment/EnvironmentManager.hpp"

namespace RbxStu {
    static std::map<const lua_TValue *, int> s_mRefsMap;
}

class Globals final : public Library {
public:
    std::string GetLibraryName() override;
    luaL_Reg *GetLibraryFunctions() override;
};
