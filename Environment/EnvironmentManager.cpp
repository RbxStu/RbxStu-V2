//
// Created by Dottik on 18/8/2024.
//

#include "EnvironmentManager.hpp"

#include <vector>
#include "Libraries/Debug.hpp"
#include "Libraries/Globals.hpp"
#include "Logger.hpp"
#include "lua.h"

std::shared_ptr<EnvironmentManager> EnvironmentManager::pInstance;

std::string Library::GetLibraryName() {
    Logger::GetSingleton()->PrintError(RbxStu::EnvironmentManager,
                                       "ERROR! Library::GetLibraryName(...) is not implemented by the inheritor!");
    throw std::exception("Function not implemented by inheritor.");
}

luaL_Reg *Library::GetLibraryFunctions() {
    Logger::GetSingleton()->PrintError(RbxStu::EnvironmentManager,
                                       "ERROR! Library::GetLibraryFunctions(...) is not implemented by the inheritor!");
    throw std::exception("Function not implemented by inheritor.");
}

std::shared_ptr<EnvironmentManager> EnvironmentManager::GetSingleton() {
    if (EnvironmentManager::pInstance == nullptr)
        EnvironmentManager::pInstance = std::make_shared<EnvironmentManager>();
    return EnvironmentManager::pInstance;
}

void EnvironmentManager::PushEnvironment(_In_ lua_State *L) {
    const auto logger = Logger::GetSingleton();

    for (const std::vector<Library *> libList = {new Debug{}, new Globals{}}; const auto &lib: libList) {
        try {
            const auto envGlobals = lib->GetLibraryFunctions();
            lua_newtable(L);
            luaL_register(L, nullptr, envGlobals);
            lua_setreadonly(L, -1, true);
            lua_setglobal(L, lib->GetLibraryName().c_str());

            lua_pushvalue(L, LUA_GLOBALSINDEX);
            luaL_register(L, nullptr, envGlobals);
            lua_pop(L, 1);

        } catch (const std::exception &ex) {
            logger->PrintError(RbxStu::EnvironmentManager,
                               std::format("Failed to initialize {} for RbxStu. Error from Lua: {}",
                                           lib->GetLibraryName(), ex.what()));
            throw;
        }

        delete lib;
    }
}
