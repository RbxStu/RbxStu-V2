//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>


#include "PacketBase.hpp"

struct HttpConfigurationPacket final : public PacketBase {
    std::string szNewFingerprint;
    std::string szNewHwid;

    HttpConfigurationPacket() { this->ulPacketId = RbxStu::WebSocketCommunication::HttpConfigurationPacket; }

    static nlohmann::json Serialize(const HttpConfigurationPacket &packet) {
        return {
                {"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"new_http_fingerprint", packet.szNewFingerprint},
                {"new_hwid", packet.szNewHwid},
        };
    }


    static HttpConfigurationPacket Deserialize(nlohmann::json json) {
        auto result = HttpConfigurationPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("new_http_fingerprint").get_to(result.szNewFingerprint);
        if (json.contains("new_hwid"))
            json.at("new_hwid").get_to(result.szNewHwid);
        return result;
    }
};
