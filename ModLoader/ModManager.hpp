//
// Created by Dottik on 7/9/2024.
//

#pragma once
#include <memory>

#include "ModBase.h"
#include "RobloxManager.hpp"

class ModManager final {
    static std::shared_ptr<ModManager> pInstance;
    std::vector<RbxStuModContext_t *> m_modList;

public:
    static std::shared_ptr<ModManager> GetSingleton();

    void LoadMods();

    RbxStuInformation_t GetRbxStuInformation(RbxStuModContext_t *modContext);
    void InitializeMods();
    void OnSchedulerInitialized(lua_State *L);

    void RegisterMod(RbxStuModContext_t *modContext);

    void UnloadMod(RbxStuModContext_t *modContext);
};
