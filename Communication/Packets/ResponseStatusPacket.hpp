//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

enum ResponseStatusPacketFlags {
    Failure = 0b0,
    Success = 0b1,
};

class ResponseStatusPacket final : public PacketBase {
public:
    __forceinline ResponseStatusPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::ResponseStatusPacket;
        this->ullPacketFlags = ResponseStatusPacketFlags::Failure;
    }

    static nlohmann::json Serialize(const ResponseStatusPacket &packet) { return PacketBase::Serialize(packet); }


    static ResponseStatusPacket Deserialize(const nlohmann::json &json) {
        auto result = ResponseStatusPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);

        return result;
    }
};
