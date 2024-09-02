//
// Created by Dottik on 29/8/2024.
//

#pragma once
#include <cstdint>
#include <memory>
#include <vector>


template<typename T>
struct CommunicationPacket {
    std::uint32_t ulPacketId;
    std::uint64_t ullPacketSize;
    T *vData;
    ~CommunicationPacket() {
        delete vData;
    }
};
