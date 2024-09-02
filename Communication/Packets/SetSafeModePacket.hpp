//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

struct SetSafeModePacket final : public PacketBase {
    bool bIsSafeMode;

    __forceinline SetSafeModePacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetSafeModePacket;
        this->bIsSafeMode = false;
    }

    static nlohmann::json Serialize(const SetSafeModePacket &packet) {
        return {{"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"enable_safe_mode", packet.bIsSafeMode}};
    }


    static SetSafeModePacket Deserialize(nlohmann::json json) {
        auto result = SetSafeModePacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("enable_safe_mode").get_to(result.bIsSafeMode);

        return result;
    }
};
