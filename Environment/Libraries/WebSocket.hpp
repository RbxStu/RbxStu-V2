//
// Created by Dottik on 31/8/2024.
//

#pragma once
#include "Environment/EnvironmentManager.hpp"
#include "ixwebsocket/IXWebSocket.h"
#include "lua.h"

class WebsocketInstance final {
    static int __index(lua_State *L);

    static int close(lua_State *L);

    static int send(lua_State *L);

public:
    WebsocketInstance *pSocketUserdata; // Self (as ud).
    ix::WebSocket *pWebSocket;

    lua_State *pLuaThread;
    long ullLuaThreadRef;

    struct {
        long onClose_ref;
        long onMessage_ref;
        long onError_ref;
    } EventReferences;
    bool bIsSchedulerReset;

    WebsocketInstance() {
        pWebSocket = new ix::WebSocket{};
        EventReferences = {-1, -1, -1};
        ullLuaThreadRef = -1;

        bIsSchedulerReset = false;
        pSocketUserdata = nullptr;
        pLuaThread = nullptr;
    }

    ~WebsocketInstance() {
        delete pWebSocket;
        pWebSocket = nullptr;
    }

    bool initialize_socket(lua_State *origin);

    bool set_callback_and_url(const std::string &url);
};

class Websocket final : public Library {
public:
    std::string GetLibraryName() override;
    luaL_Reg *GetLibraryFunctions() override;
    static void ResetWebsockets();
};
