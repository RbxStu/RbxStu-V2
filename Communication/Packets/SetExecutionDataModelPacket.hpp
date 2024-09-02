//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

enum SetExecutionDataModelPacketFlags {
    Standalone = 0b0,
    EditMode = 0b1,
    Client = 0b10,
    Server = 0b100,
};

struct SetExecutionDataModelPacket final : public PacketBase {
    __forceinline SetExecutionDataModelPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetExecutionDataModelPacket;
        this->ullPacketFlags = SetExecutionDataModelPacketFlags::Client;
    }
};
