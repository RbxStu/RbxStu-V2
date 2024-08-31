//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

struct SetFunctionBlockStatePacket final : public PacketBase {
public:
    bool bisFunctionBlocked;
    const char szFunctionName[0xFF];
};
