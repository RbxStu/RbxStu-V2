//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

enum ResponseStatusPacketFlags {
    ResponseStatusFailure = 0b0,
    ResponseStatusSuccess = 0b1,
};

class ResponseStatusPacket final : public PacketBase {
public:
    std::string szStatusText;
    __forceinline ResponseStatusPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::ResponseStatusPacket;
        this->ullPacketFlags = ResponseStatusPacketFlags::ResponseStatusFailure;
    }

    static nlohmann::json Serialize(const ResponseStatusPacket &packet) {
        return {{"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"status_text", packet.szStatusText}};
    }


    static ResponseStatusPacket Deserialize(const nlohmann::json &json) {
        auto result = ResponseStatusPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("status_text").get_to(result.szStatusText);

        return result;
    }
};
