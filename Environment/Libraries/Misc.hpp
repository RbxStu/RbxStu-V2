//
// Created by Pixeluted on 22/08/2024.
//

#pragma once
#include "Environment/EnvironmentManager.hpp"

namespace RbxStu {
    namespace Misc {
        int request(lua_State *L);
    }
} // namespace RbxStu

class Misc final : public Library {
public:
    std::string GetLibraryName() override;
    luaL_Reg *GetLibraryFunctions() override;
};
