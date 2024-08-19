//
// Created by Dottik on 18/8/2024.
//

#include "EnvironmentManager.hpp"

#include "Libraries/Globals.hpp"
#include "Logger.hpp"
#include "lua.h"

std::shared_ptr<EnvironmentManager> EnvironmentManager::pInstance;

std::string Library::GetLibraryName() const {
    Logger::GetSingleton()->PrintError(RbxStu::EnvironmentManager,
                                       "ERROR! Library::GetLibraryName(...) is not implemented by the inheritor!");
    throw std::exception("Function not implemented by inheritor.");
}

std::int32_t Library::GetFunctionCount() const {
    Logger::GetSingleton()->PrintError(RbxStu::EnvironmentManager,
                                       "ERROR! Library::GetFunctionCount(...) is not implemented by the inheritor!");
    throw std::exception("Function not implemented by inheritor.");
}

luaL_Reg *Library::GetLibraryFunctions() const {
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
    const auto globals = Globals{};
    auto envGlobals = globals.GetLibraryFunctions();

    try {
        lua_newtable(L);
        luaL_register(L, nullptr, envGlobals);
        lua_setglobal(L, globals.GetLibraryName().c_str());
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        luaL_register(L, nullptr, envGlobals);
        lua_pop(L, 1);
    } catch (const std::exception &ex) {
        logger->PrintError(RbxStu::EnvironmentManager,
                           std::format("Failed to initialize globals for RbxStu. Error from Lua: {}", ex.what()));
        throw;
    }
}
