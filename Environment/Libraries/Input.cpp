//
// Created by Pixeluted on 22/08/2024.
//

#include "Input.hpp"
#include <Windows.h>

namespace RbxStu {
    namespace Input {
        int isrbxactive(lua_State *L) {
            lua_pushboolean(L, GetForegroundWindow() == GetCurrentProcess());
            return 1;
        }
    } // namespace Input
} // namespace RbxStu

std::string Input::GetLibraryName() { return "input"; };
luaL_Reg *Input::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"isrbxactive", RbxStu::Input::isrbxactive},
                                    {"isgameactive", RbxStu::Input::isrbxactive},

                                    {nullptr, nullptr}};

    return reg;
}
