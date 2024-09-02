//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

enum SetFastVariablePacketFlag { Boolean = 0b0, Integer = 0b1, String = 0b10, Unassigned = 0b100 };

struct SetFastVariablePacket final : public PacketBase {
    union {
        const char szNewValue[0xFF];
        const int lNewValue;
        const bool bNewValue;
    } newValue;
    const char szFastVariableName[0xFF];

    __forceinline SetFastVariablePacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetFastVariablePacket;
        this->ullPacketFlags = SetFastVariablePacketFlag::Unassigned;
    }
};
