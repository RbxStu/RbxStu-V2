//
// Created by Dottik on 18/8/2024.
//

#pragma once
#include <memory>

#include "lstate.h"
#include "lualib.h"

__interface Library {
    virtual std::string GetLibraryName();
    virtual luaL_Reg *GetLibraryFunctions();
};

class EnvironmentManager final {
    static std::shared_ptr<EnvironmentManager> pInstance;

public:
    static std::shared_ptr<EnvironmentManager> GetSingleton();

    void SetFunctionBlocked(const std::string &functionName, bool status);
    void SetServiceBlocked(const std::string &serviceName, bool status);

    void PushEnvironment(lua_State *L);
};
