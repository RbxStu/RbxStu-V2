//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>


#include "PacketBase.hpp"

struct SetRequestFingerprintPacket final : public PacketBase {
    char szNewFingerprint[0xFF]{};

    SetRequestFingerprintPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetRequestFingerprintPacket;
        memset(this->szNewFingerprint, '1', sizeof(this->szNewFingerprint));
        this->szNewFingerprint[0xFE] = 0;
    }
};
