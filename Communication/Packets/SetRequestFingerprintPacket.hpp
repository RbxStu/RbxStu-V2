//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

struct SetRequestFingerprintPacket final : public PacketBase {
public:
    const char szNewFingerprint[0xFF];
};
