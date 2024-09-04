//
// Created by Dottik on 3/9/2024.
//

#pragma once
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include "PacketBase.hpp"

enum SetScriptSourceAccessPacketFlags {
    DisallowSourceAccess = 0b0,
    AllowSourceAccess = 0b1,
};

class SetScriptSourceAccessPacket final : public PacketBase {
public:
    __forceinline SetScriptSourceAccessPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetScriptSourceAccessPacket;
        this->ullPacketFlags = SetScriptSourceAccessPacketFlags::DisallowSourceAccess;
    }

    static nlohmann::json Serialize(const SetScriptSourceAccessPacket &packet) {
        return {{"packet_id", packet.ulPacketId}, {"packet_flags", packet.ullPacketFlags}};
    }


    static SetScriptSourceAccessPacket Deserialize(const nlohmann::json &json) {
        auto result = SetScriptSourceAccessPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);

        return result;
    }
};
