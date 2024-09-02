//
// Created by Dottik on 29/8/2024.
//

#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include "Packets/PacketBase.hpp"
#include "Utilities.hpp"

class PacketSerdes final {
    static std::shared_ptr<PacketSerdes> pInstance;

public:
    PacketSerdes() = default;

    static std::shared_ptr<PacketSerdes> GetSingleton();

    template<RbxStu::Concepts::TypeConstraint<PacketFunctions> T>
    __forceinline std::optional<T> DeserializeFromJson(const std::string &input) {
        try {
            return T::Deserialize(nlohmann::json::parse(input));
        } catch (const std::exception &ex) {
            Logger::GetSingleton()->PrintError(RbxStu::PacketSerdes,
                                               std::format("Failed to deserialize packet json! Structure Name: {}; "
                                                           "Provided Input: \n\n{}\n\nerror.what(): {}",
                                                           typeid(T).name(), input, ex.what()));
            return {};
        }
    }

    template<RbxStu::Concepts::TypeConstraint<PacketFunctions> T>
    __forceinline nlohmann::json SerializeFromStructure(const T &structure) {
        try {
            return T::Serialize(structure);
        } catch (const std::exception &ex) {
            Logger::GetSingleton()->PrintError(
                    RbxStu::PacketSerdes,
                    std::format("Failed to serialize packet structure! Structure Name : \n\n{}\n\nerror.what(): {}",
                                typeid(T).name(), ex.what()));
            return {};
        }
    }

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    __forceinline static bool ContainsFlag(T packet, std::uint64_t flag) {
        return (packet.ullPacketFlags & flag) == flag;
    }

    __forceinline std::int32_t ReadPacketIdentifier(const std::string &str) {
        return PacketBase::Deserialize(nlohmann::json::parse(str)).ulPacketId;
    }
};
