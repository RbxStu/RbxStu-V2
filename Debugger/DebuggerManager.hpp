//
// Created by Dottik on 15/9/2024.
//

#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Scanner.hpp"
#include "lstate.h"

struct DebuggerInformation {
    bool bBreakpointOnEntry;
    std::vector<std::uint32_t> vecBreakpointList;
};

class DebuggerManager {
    static std::shared_ptr<DebuggerManager> pInstance;

    std::atomic<bool> m_bIsInitialized;
    std::atomic<bool> m_bObtainedCallbacks;

    std::map<std::string, void *> m_mapDebugManagerFunctions;

    std::map<std::string, lua_State *> m_mapScriptStateMap;

    std::map<std::string, std::shared_ptr<DebuggerInformation>> m_mapDebuggerInformationMap;

public:
    void Initialize();
    void PushScriptTracking(const char *chunkname, lua_State *scriptState);

    lua_State *GetScriptByChunkName(const std::string &str) const {
        if (this->m_mapScriptStateMap.contains(str))
            return this->m_mapScriptStateMap.at(str);
        return nullptr;
    };

    static std::shared_ptr<DebuggerManager> GetSingleton();

    bool IsInitialized() const;
    void RegisterCallbackCopy(const lua_Callbacks *callbacks);


    bool IsLocalPlayerScript(const std::string_view &chunkName) const;
    bool IsServerScript(const std::string_view &chunkName) const;
    bool IsBreakpointOnEntry(const std::string &chunkName) const;
    bool IsBreakpointOnEntry(const std::string &chunkName);
};
