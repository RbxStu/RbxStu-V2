//
// Created by Dottik on 13/8/2024.
//

#pragma once
#include <map>
#include <memory>
#include <string>

class LuauManager final {
    static std::shared_ptr<LuauManager> pInstance;

    std::map<std::string, void *> m_mapHookMap;
    std::map<std::string, void *> m_mapLuauFunctions;

    bool m_bIsInitialized = false;

public:
    static std::shared_ptr<LuauManager> GetSingleton();
    void *GetHookOriginal(const std::string &functionName);
    void Initialize();
};
