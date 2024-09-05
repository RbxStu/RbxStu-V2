//
// Created by Pixeluted on 20/08/2024.
//
#include "Filesystem.hpp"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "Logger.hpp"
#include "RobloxManager.hpp"
#include "Scheduler.hpp"
#include "ldebug.h"

namespace fs = std::filesystem;

bool canBeUsed = false;
std::filesystem::path workspaceDir;
std::unordered_set<std::string> blacklistedExtensions = {".exe", ".dll", ".bat", ".cmd", ".scr", ".vbs", ".js",
                                                         ".ts",  ".wsf", ".msi", ".com", ".lnk", ".ps1", ".py",
                                                         ".py3", ".pyc", ".pyw", ".scr", ".msi", ".html"};

std::string GetDllDir() {
    char path[MAX_PATH];
    HMODULE hModule = NULL;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&GetDllDir), &hModule)) {
        if (GetModuleFileNameA(hModule, path, sizeof(path))) {
            const std::filesystem::path fullPath(path);
            return fullPath.parent_path().string();
        }
    }
    return "";
}

bool IsPathSafe(const std::string &relativePath) {
    const fs::path base = fs::absolute(workspaceDir);
    const fs::path combined = base / relativePath;

    const fs::path normalizedBase = base.lexically_normal();
    const fs::path normalizedCombined = combined.lexically_normal();

    const auto baseStr = normalizedBase.string();
    const auto combinedStr = normalizedCombined.string();

    if (combinedStr.compare(0, baseStr.length(), baseStr) != 0)
        return false;

    std::string lowerRelativePath = relativePath;
    std::ranges::transform(lowerRelativePath, lowerRelativePath.begin(), ::tolower);

    if (lowerRelativePath.find("..") != std::string::npos)
        return false;

    return true;
}

__forceinline void CanBeUsed(lua_State *L) {
    if (!canBeUsed)
        luaG_runerror(L, "Filesystem functions are disabled!");
}

__forceinline std::filesystem::path CheckPath(lua_State *L) {
    const auto relativePath = lua_tostring(L, 1);

    if (!IsPathSafe(relativePath))
        luaG_runerror(L, "This path is unsafe!");

    return workspaceDir / relativePath;
}

namespace RbxStu {
    namespace Filesystem {
        int isfile(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);

            lua_pushboolean(L, fs::exists(absolutePath) && fs::is_regular_file(absolutePath));
            return 1;
        }

        int isfolder(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);

            lua_pushboolean(L, fs::exists(absolutePath) && fs::is_directory(absolutePath));
            return 1;
        }

        int listfiles(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);

            if (!fs::exists(absolutePath) || !fs::is_directory(absolutePath)) {
                luaG_runerror(L, "This folder doesn't exist or it's not a folder!");
            }

            lua_newtable(L);
            int currentIndex = 1;
            for (const auto &entry: fs::directory_iterator(absolutePath)) {
                lua_pushstring(L, entry.path().string().c_str());
                lua_rawseti(L, -2, currentIndex);
                currentIndex++;
            }

            return 1;
        }

        int readfile(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);

            if (!fs::exists(absolutePath) || !fs::is_regular_file(absolutePath)) {
                luaG_runerror(L, "This file doesn't exist or it's not a file!");
            }

            std::ifstream file(absolutePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                luaG_runerror(L, "Failed to open the file!");
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<char> buffer(size);
            if (!file.read(buffer.data(), size)) {
                luaG_runerror(L, "Failed to read file!");
            }

            lua_pushlstring(L, buffer.data(), size);

            return 1;
        }

        int writefile(lua_State *L) {
            luaL_checkstring(L, 1);
            luaL_checkstring(L, 2);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);
            size_t contentLength;
            auto fileContent = lua_tolstring(L, 2, &contentLength);

            if (fs::is_directory(absolutePath)) {
                luaG_runerror(L, "There is a directory at this path!");
            }

            if (absolutePath.extension().empty()) {
                luaG_runerror(L, "Filename must have an extension");
            }

            std::string ext = absolutePath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (blacklistedExtensions.find(ext) != blacklistedExtensions.end()) {
                luaG_runerror(L, "This file extension is not allowed");
            }

            fs::create_directories(absolutePath.parent_path());

            std::ofstream file(absolutePath, std::ios::binary);
            if (!file.is_open()) {
                luaG_runerror(L, "Failed to open the file!");
            }

            file.write(fileContent, contentLength);
            if (file.fail()) {
                luaG_runerror(L, "Failed to write the content!");
            }

            file.close();
            if (file.fail()) {
                luaG_runerror(L, "Failed to close the file!");
            }

            return 0;
        }

        int makefolder(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);

            if (fs::is_directory(absolutePath)) {
                // luaG_runerror(L, "There is already a folder on this path!"); Most scripts do not think, that this
                // will ever error, so I have to suppress this
                return 0;
            }

            if (fs::is_regular_file(absolutePath)) {
                luaG_runerror(L, "There is a file on this path!");
            }

            if (fs::exists(absolutePath)) {
                luaG_runerror(L, "There is already something on this path!");
            }

            fs::create_directories(absolutePath);
            return 0;
        }

        int delfile(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);

            if (!fs::exists(absolutePath) || !fs::is_regular_file(absolutePath)) {
                luaG_runerror(L, "This file doesn't exist or it's not a file!");
            }

            fs::remove(absolutePath);
            return 0;
        }

        int delfolder(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto absolutePath = CheckPath(L);

            if (!fs::exists(absolutePath) || !fs::is_directory(absolutePath)) {
                luaG_runerror(L, "This folder doesn't exist or it's not a folder!");
            }

            fs::remove_all(absolutePath);
            return 0;
        }

        int dofile(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto relativePath = lua_tostring(L, 1);
            auto absolutePath = CheckPath(L);

            if (!fs::exists(absolutePath) || !fs::is_regular_file(absolutePath)) {
                luaG_runerror(L, "This file doesn't exist or it's not a file!");
            }

            lua_pushcfunction(L, RbxStu::Filesystem::readfile, "DoFile");
            lua_pushstring(L, relativePath);
            lua_call(L, 1, 1);
            auto scriptContent = lua_tostring(L, -1);
            Scheduler::GetSingleton()->ScheduleJob(SchedulerJob(scriptContent));
            lua_pop(L, 1);

            return 0;
        }

        int appendfile(lua_State *L) {
            luaL_checkstring(L, 1);
            luaL_checkstring(L, 2);
            CanBeUsed(L);

            auto relativePath = lua_tostring(L, 1);
            auto contentToAppend = lua_tostring(L, 2);
            auto absolutePath = CheckPath(L);

            if (!fs::exists(absolutePath) || !fs::is_regular_file(absolutePath)) {
                luaG_runerror(L, "This file doesn't exist or it's not a file!");
            }

            lua_pushcfunction(L, RbxStu::Filesystem::readfile, "AppendFile");
            lua_pushstring(L, relativePath);
            lua_call(L, 1, 1);
            auto currentContent = _strdup(lua_tostring(L, -1)); // Clone string bc it will get GC'ed once we pop it
            lua_pop(L, 1);
            auto newContent = std::format("{}{}", currentContent, contentToAppend).c_str();
            lua_pushcfunction(L, RbxStu::Filesystem::writefile, "AppendFile");
            lua_pushstring(L, relativePath);
            lua_pushstring(L, newContent);
            lua_call(L, 2, 0);

            return 0;
        }

        int loadfile(lua_State *L) {
            luaL_checkstring(L, 1);
            CanBeUsed(L);

            auto relativePath = lua_tostring(L, 1);
            auto absolutePath = CheckPath(L);
            const char *sectionName = "";
            if (!lua_isnil(L, 2)) {
                sectionName = lua_tostring(L, 2);
            }

            if (!fs::exists(absolutePath) || !fs::is_regular_file(absolutePath)) {
                luaG_runerror(L, "This file doesn't exist or it's not a file!");
            }


            lua_pushcfunction(L, RbxStu::Filesystem::readfile, "loadfile");
            lua_pushstring(L, relativePath);
            lua_call(L, 1, 1);
            auto codeContent = _strdup(lua_tostring(L, -1)); // Copy bc once we pop, it's going to get GC'ed
            lua_pop(L, 1);

            lua_getglobal(L, "loadstring");
            lua_pushstring(L, codeContent);
            lua_pushstring(L, sectionName);
            if (lua_pcall(L, 2, 2, 0) != LUA_OK)
                return 2;

            lua_pop(L, 1); // Due to the possibility of having the two values, we know a success marks that we have
                           // nothing on second return, but something on first, so we must pop.

            return 1;
        }
    } // namespace Filesystem
} // namespace RbxStu

std::string Filesystem::GetLibraryName() { return "fs"; }

luaL_Reg *Filesystem::GetLibraryFunctions() {
    const auto logger = Logger::GetSingleton();
    if (const auto currentDirectory = fs::path(GetDllDir()); !currentDirectory.empty()) {
        logger->PrintDebug(RbxStu::Env_Filesystem, std::format("Current path: {}", currentDirectory.string()));
        canBeUsed = true;

        workspaceDir = currentDirectory / "workspace";
        if (!fs::exists(workspaceDir)) {
            if (fs::create_directory(workspaceDir)) {
                logger->PrintInformation(RbxStu::Env_Filesystem,
                                         std::format("Created workspace directory at: {}", workspaceDir.string()));
            } else {
                canBeUsed = false;
                logger->PrintWarning(RbxStu::Env_Filesystem,
                                     "Failed to create workspace directory! Filesystem functions will be disabled!");
            }
        }
    } else {
        logger->PrintWarning(RbxStu::Env_Filesystem,
                             "Failed to get directory path of the dll! Filesystem functions will be disabled!");
    }

    const auto reg = new luaL_Reg[]{
            {"isfile", RbxStu::Filesystem::isfile},
            {"isfolder", RbxStu::Filesystem::isfolder},
            {"listfiles", RbxStu::Filesystem::listfiles},
            {"readfile", RbxStu::Filesystem::readfile},
            {"writefile", RbxStu::Filesystem::writefile},
            {"makefolder", RbxStu::Filesystem::makefolder},
            {"delfile", RbxStu::Filesystem::delfile},
            {"delfolder", RbxStu::Filesystem::delfolder},
            {"dofile", RbxStu::Filesystem::dofile},
            {"appendfile", RbxStu::Filesystem::appendfile},
            {"loadfile", RbxStu::Filesystem::loadfile},

            {nullptr, nullptr},
    };

    return reg;
}
