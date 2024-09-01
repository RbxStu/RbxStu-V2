//
// Created by Dottik on 29/8/2024.
//

#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <type_traits>

#include "CommunicationPacket.hpp"
#include "Packets/PacketBase.hpp"
#include "Utilities.hpp"

class PacketSerdes final {
    static std::shared_ptr<PacketSerdes> pInstance;
    PacketSerdes();

public:
    std::shared_ptr<PacketSerdes> GetSingleton();
    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    std::optional<CommunicationPacket<T>> DeserializeFromString(const std::string &input);

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    std::optional<CommunicationPacket<T>> DeserializeFromBytes(const std::vector<std::uint8_t> &input);

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    __forceinline static bool ContainsFlag(CommunicationPacket<T> packet, std::uint64_t flag) {
        return (packet.vData->ullPacketFlags & flag) == flag;
    }
};
