//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

enum ResponseStatusPacketFlags {
    Failure = 0b0,
    Success = 0b1,
};

struct ResponseStatusPacket final : public PacketBase {
    __forceinline ResponseStatusPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::ResponseStatusPacket;
        this->ullPacketFlags = ResponseStatusPacketFlags::Failure;
    }
};
