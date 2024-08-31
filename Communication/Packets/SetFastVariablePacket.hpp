//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

enum SetFlagVariablePacketFlag {

};

struct SetFastVariablePacket final : public PacketBase {
public:
    union {
        const char szNewValue[0xFF];
        const int lNewValue;
        const bool bNewValue;
    } newValue;
    const char szFastVariableName[0xFF];
};
