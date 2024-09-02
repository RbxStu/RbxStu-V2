//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

struct SetSafeModePacket final : public PacketBase {
    bool bIsSafeMode;

    __forceinline SetSafeModePacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetSafeModePacket;
        this->bIsSafeMode = false;
    }
};
