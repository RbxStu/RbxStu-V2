//
// Created by Dottik on 18/8/2024.
//

#pragma once
#include <memory>

#include "lstate.h"
#include "lualib.h"

class Library abstract {
protected:
    ~Library() = default;

public:
    [[nodiscard]] virtual std::string GetLibraryName() const;
    [[nodiscard]] virtual std::int32_t GetFunctionCount() const;
    [[nodiscard]] virtual luaL_Reg *GetLibraryFunctions() const;
};

class EnvironmentManager final {
    static std::shared_ptr<EnvironmentManager> pInstance;

public:
    static std::shared_ptr<EnvironmentManager> GetSingleton();
    void PushEnvironment(lua_State *L);
};
