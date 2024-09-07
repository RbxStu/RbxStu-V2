//
// Created by Dottik on 18/8/2024.
//

#pragma once
#include <memory>

#include "lstate.h"
#include "lualib.h"
#include <vector>

__interface Library {
    virtual std::string GetLibraryName();
    virtual luaL_Reg *GetLibraryFunctions();
};

class FlexibleLibrary final : public Library {
    luaL_Reg *functions;
    std::string libname;

public:
    ~FlexibleLibrary() {
        free(functions); // High chance its C allocated!
    }

    FlexibleLibrary(const char *libname, luaL_Reg *libreg) {
        this->libname = libname;
        this->functions = libreg;
    }

    std::string GetLibraryName() override { return this->libname; }
    luaL_Reg *GetLibraryFunctions() override { return this->functions; }
};

class EnvironmentManager final {
    static std::shared_ptr<EnvironmentManager> pInstance;
    std::vector<Library *> m_vLibraryList;

public:
    static std::shared_ptr<EnvironmentManager> GetSingleton();

    void SetFunctionBlocked(const std::string &functionName, bool status);
    void SetServiceBlocked(const std::string &serviceName, bool status);

    void DeclareLibrary(const char *libname, luaL_Reg *functionList);

    void PushEnvironment(lua_State *L);
};
