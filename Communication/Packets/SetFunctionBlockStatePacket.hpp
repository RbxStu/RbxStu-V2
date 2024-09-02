//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>


#include "PacketBase.hpp"

struct SetFunctionBlockStatePacket final : public PacketBase {
    bool bisFunctionBlocked;
    char szFunctionName[0xFF];

    __forceinline SetFunctionBlockStatePacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetFunctionBlockStatePacket;
        memset(this->szFunctionName, '1', sizeof(this->szFunctionName));
        this->szFunctionName[0xFE] = 0;
        this->bisFunctionBlocked = false;
    }
};
