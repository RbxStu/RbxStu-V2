//
// Created by Pixeluted on 22/08/2024.
//

#include "Console.hpp"

#include <sstream>
#include <Windows.h>

#include "Logger.hpp"
#include "RobloxManager.hpp"

namespace RbxStu {
    namespace Console {
        int rconsolecreate(lua_State *L) {
            AllocConsole();
            return 0;
        }

        int rconsoledestroy(lua_State *L) {
            FreeConsole();
            return 0;
        }

        int rconsolesettitle(lua_State *L) {
            auto wndName = luaL_checkstring(L, 1);
            SetWindowTextA(GetConsoleWindow(), wndName);
            return 0;
        }

        int rconsoleprint(lua_State *L) {
            auto argc = lua_gettop(L);
            std::stringstream strStream;
            strStream << "[rconsoleprint] ";
            for (int i = 0; i <= argc - 1; i++) {
                const char *lStr = luaL_tolstring(L, i + 1, nullptr);
                strStream << lStr << " ";
            }
            Logger::GetSingleton()->PrintInformation(RbxStu::RobloxConsole, strStream.str());
        }

        int rconsolewarn(lua_State *L) {
            auto argc = lua_gettop(L);
            std::stringstream strStream;
            strStream << "[rconsolewarn] ";
            for (int i = 0; i <= argc - 1; i++) {
                const char *lStr = luaL_tolstring(L, i + 1, nullptr);
                strStream << lStr << " ";
            }
            Logger::GetSingleton()->PrintWarning(RbxStu::RobloxConsole, strStream.str());
        }

        int rconsoleerror(lua_State *L) {
            auto argc = lua_gettop(L);
            std::stringstream strStream;
            strStream << "[rconsoleerror] ";
            for (int i = 0; i <= argc - 1; i++) {
                const char *lStr = luaL_tolstring(L, i + 1, nullptr);
                strStream << lStr << " ";
            }
            Logger::GetSingleton()->PrintError(RbxStu::RobloxConsole, strStream.str());
            luaL_error(L, strStream.str().c_str());
        }
    }
}

std::string Console::GetLibraryName() { return "console"; }
luaL_Reg *Console::GetLibraryFunctions() {
    auto reg = new luaL_Reg[] {
        {"rconsolecreate", RbxStu::Console::rconsolecreate},
        {"rconsoledestroy", RbxStu::Console::rconsoledestroy},
        {"rconsolesettitle", RbxStu::Console::rconsolesettitle},
        {"rconsolename", RbxStu::Console::rconsolesettitle},
        {"rconsoleprint", RbxStu::Console::rconsoleprint},
        {"rconsolewarn", RbxStu::Console::rconsolewarn},
        {"rconsoleerror", RbxStu::Console::rconsoleerror},

        {"consolecreate", RbxStu::Console::rconsolecreate},
        {"consoledestroy", RbxStu::Console::rconsoledestroy},
        {"consolesettitle", RbxStu::Console::rconsolesettitle},
        {"consoleprint", RbxStu::Console::rconsoleprint},
        {"consolewarn", RbxStu::Console::rconsolewarn},
        {"consoleerror", RbxStu::Console::rconsoleerror},

        {nullptr, nullptr}
    };

    return reg;
}
