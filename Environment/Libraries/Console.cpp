//
// Created by Pixeluted on 22/08/2024.
//

#include "Console.hpp"

#include <Windows.h>
#include <sstream>

#include "Logger.hpp"
#include "RobloxManager.hpp"

namespace RbxStu {
    namespace Console {
        HWND CreateIfNotCreated() {
            if (const auto hWnd = GetConsoleWindow(); !hWnd || hWnd == INVALID_HANDLE_VALUE) {
                AllocConsole();
                Logger::GetSingleton()->OpenStandard();
                SetConsoleTitleA("-- RbxStu V2 --");
                Logger::GetSingleton()->PrintInformation(RbxStu::Anonymous, "-- roblox console created --");
            }

            return GetConsoleWindow();
        }

        int rconsolecreate(lua_State *L) {
            CreateIfNotCreated();
            return 0;
        }

        int rconsoledestroy(lua_State *L) {
            if (const auto hWnd = GetConsoleWindow(); hWnd && hWnd != INVALID_HANDLE_VALUE) {
                ShowWindow(hWnd, 0);
                FreeConsole();
            }

            return 0;
        }

        int rconsolesettitle(lua_State *L) {
            CreateIfNotCreated();
            const auto wndName = luaL_checkstring(L, 1);
            SetConsoleTitleA(wndName);
            return 0;
        }

        int rconsoleprint(lua_State *L) {
            CreateIfNotCreated();
            const auto argc = lua_gettop(L);
            std::stringstream strStream;
            strStream << "[rconsoleprint] ";
            for (int i = 0; i <= argc - 1; i++) {
                const char *lStr = luaL_tolstring(L, i + 1, nullptr);
                strStream << lStr << " ";
            }
            Logger::GetSingleton()->PrintInformation(RbxStu::RobloxConsole, strStream.str());

            return 0;
        }

        int rconsolewarn(lua_State *L) {
            CreateIfNotCreated();
            const auto argc = lua_gettop(L);
            std::stringstream strStream;
            strStream << "[rconsolewarn] ";
            for (int i = 0; i <= argc - 1; i++) {
                const char *lStr = luaL_tolstring(L, i + 1, nullptr);
                strStream << lStr << " ";
            }
            Logger::GetSingleton()->PrintWarning(RbxStu::RobloxConsole, strStream.str());

            return 0;
        }

        int rconsoleerror(lua_State *L) {
            CreateIfNotCreated();
            const auto argc = lua_gettop(L);
            std::stringstream strStream;
            strStream << "[rconsoleerror] ";
            for (int i = 0; i <= argc - 1; i++) {
                const char *lStr = luaL_tolstring(L, i + 1, nullptr);
                strStream << lStr << " ";
            }
            Logger::GetSingleton()->PrintError(RbxStu::RobloxConsole, strStream.str());

            return 0;
        }
    } // namespace Console
} // namespace RbxStu

std::string Console::GetLibraryName() { return "console"; }
luaL_Reg *Console::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"rconsolecreate", RbxStu::Console::rconsolecreate},
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

                                    {nullptr, nullptr}};

    return reg;
}
