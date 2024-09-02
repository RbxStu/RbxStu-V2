//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>


#include "PacketBase.hpp"

enum ScheduleLuauResponsePacketFlags {
    Failure = 0b0,
    Success = 0b1,
};

class ScheduleLuauResponsePacket final : public PacketBase {
public:
    std::string szText;
    std::string szOperationIdentifier;
    __forceinline ScheduleLuauResponsePacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::ScheduleLuauResponsePacket;
        this->ullPacketFlags = ScheduleLuauResponsePacketFlags::Failure;
    }

    static nlohmann::json Serialize(const ScheduleLuauResponsePacket &packet) {
        return {{"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"result", packet.szText},
                {"_id", packet.szOperationIdentifier}};
    }


    static ScheduleLuauResponsePacket Deserialize(const nlohmann::json &json) {
        auto result = ScheduleLuauResponsePacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("result").get_to(result.szText);
        json.at("_id").get_to(result.szOperationIdentifier);

        return result;
    }
};
