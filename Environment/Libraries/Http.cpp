//
// Created by Dottik on 1/9/2024.
//

#include "Http.hpp"
#include "Misc.hpp"

std::string Http::GetLibraryName() { return "http"; }

luaL_Reg *Http::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"request", RbxStu::Misc::request},

                                    {nullptr, nullptr}};

    return reg;
}
