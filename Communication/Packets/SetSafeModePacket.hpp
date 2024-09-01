//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

struct SetSafeModePacket final : public PacketBase {
public:
    bool bIsSafeMode;
};
