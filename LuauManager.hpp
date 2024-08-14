//
// Created by Dottik on 13/8/2024.
//

#pragma once
#include <map>
#include <memory>
#include <string>

class LuauManager final {
    std::map<std::string, void *> m_mapLuauFunctions;

    bool m_bIsInitialized;

public:
    static std::shared_ptr<LuauManager> GetSingleton();
    void Initialize();
};
