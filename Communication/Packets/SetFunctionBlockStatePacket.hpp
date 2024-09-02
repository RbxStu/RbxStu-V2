//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>


#include "PacketBase.hpp"

enum SetFunctionBlockStatePacketFlags {
    Block = 0x0,
    Allow = 0x1,
};

struct SetFunctionBlockStatePacket final : public PacketBase {
    std::string szFunctionName;

    __forceinline SetFunctionBlockStatePacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetFunctionBlockStatePacket;
        this->ullPacketFlags = SetFunctionBlockStatePacketFlags::Block;
    }

    static nlohmann::json Serialize(const SetFunctionBlockStatePacket &packet) {
        nlohmann::json json = {
                {"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"function_name", packet.szFunctionName},
        };
    }


    static SetFunctionBlockStatePacket Deserialize(const nlohmann::json &json) {
        auto result = SetFunctionBlockStatePacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("function_name").get_to(result.szFunctionName);

        return result;
    }
};
