//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>


#include "PacketBase.hpp"

struct SetRequestFingerprintPacket final : public PacketBase {
    std::string szNewFingerprint;

    SetRequestFingerprintPacket() { this->ulPacketId = RbxStu::WebSocketCommunication::SetRequestFingerprintPacket; }

    static nlohmann::json Serialize(const SetRequestFingerprintPacket &packet) {
        return {{"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"new_http_fingerprint", packet.szNewFingerprint}};
    }


    static SetRequestFingerprintPacket Deserialize(nlohmann::json json) {
        auto result = SetRequestFingerprintPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("new_http_fingerprint").get_to(result.szNewFingerprint);

        return result;
    }
};
