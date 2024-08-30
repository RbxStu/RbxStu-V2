//
// Created by Dottik on 29/8/2024.
//

#pragma once
#include <cstdint>
#include <vector>


struct CommunicationPacket {
    std::uint32_t ulPacketId;
    std::uint64_t ullPacketSize;
    std::vector<std::uint8_t> vData;
};
