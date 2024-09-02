//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

struct SetNativeCodeGenPacket final : public PacketBase {
    bool bEnableNativeCodeGen;

    SetNativeCodeGenPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetNativeCodeGenPacket;
        this->bEnableNativeCodeGen = false;
    }
};
