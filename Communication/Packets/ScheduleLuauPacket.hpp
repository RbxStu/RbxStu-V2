//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "PacketBase.hpp"

class ScheduleLuauPacket final : public PacketBase {
public:
    std::string szLuauCode;
    std::string szOperationIdentifier;

    __forceinline ScheduleLuauPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::ScheduleLuauPacket;
        this->szLuauCode = "";
    }

    static nlohmann::json Serialize(const ScheduleLuauPacket &packet) {
        return {{"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"luau_code", packet.szLuauCode},
                {"_id", packet.szOperationIdentifier}};
    }

    static ScheduleLuauPacket Deserialize(nlohmann::json json) {
        auto result = ScheduleLuauPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("luau_code").get_to(result.szLuauCode);
        json.at("_id").get_to(result.szOperationIdentifier);
        return result;
    }
};
