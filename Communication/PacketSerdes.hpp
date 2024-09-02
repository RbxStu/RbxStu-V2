//
// Created by Dottik on 29/8/2024.
//

#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include "CommunicationPacket.hpp"
#include "Packets/PacketBase.hpp"
#include "Utilities.hpp"

class PacketSerdes final {
    static std::shared_ptr<PacketSerdes> pInstance;

public:
    PacketSerdes() = default;

    static std::shared_ptr<PacketSerdes> GetSingleton();

    template<RbxStu::Concepts::TypeConstraint<PacketFunctions> T>
    __forceinline std::optional<T> DeserializeFromJson(const std::string &input) {
        return T::Deserialize(nlohmann::json::parse(input));
    }

    template<RbxStu::Concepts::TypeConstraint<PacketFunctions> T>
    __forceinline nlohmann::json SerializeFromStructure(const T &structure) {
        return T::Serialize(structure);
    }

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    __forceinline static bool ContainsFlag(CommunicationPacket<T> packet, std::uint64_t flag) {
        return (packet.vData->ullPacketFlags & flag) == flag;
    }

    __forceinline std::int32_t ReadPacketIdentifier(const std::string &str) {
        return PacketBase::Deserialize(nlohmann::json::parse(str)).ulPacketId;
    }
};
