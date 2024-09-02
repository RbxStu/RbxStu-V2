//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>


#include "PacketBase.hpp"

enum SetExecutionDataModelPacketFlags {
    Edit = 0b0,
    Client = 0b1,
    Server = 0b10,
    Standalone = 0b100,
};

struct SetExecutionDataModelPacket final : public PacketBase {
    __forceinline SetExecutionDataModelPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetExecutionDataModelPacket;
        this->ullPacketFlags = SetExecutionDataModelPacketFlags::Client;
    }

    static nlohmann::json Serialize(const SetExecutionDataModelPacket &packet) { return PacketBase::Serialize(packet); }

    static SetExecutionDataModelPacket Deserialize(const nlohmann::json &json) {
        auto result = SetExecutionDataModelPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);

        return result;
    }
};
