//
// Created by Dottik on 29/8/2024.
//

#pragma once

#include <optional>
#include <string>

#include "CommunicationPacket.hpp"

class PacketSerdes final {

public:
    std::optional<CommunicationPacket> DeserializeFromString(const std::string &input);
    std::optional<CommunicationPacket> DeserializeFromBytes(const std::vector<std::uint8_t> &input);
};
