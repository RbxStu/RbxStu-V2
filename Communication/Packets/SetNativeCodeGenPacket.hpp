//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include "PacketBase.hpp"

struct SetNativeCodeGenPacket final : public PacketBase {
    bool bEnableNativeCodeGen;

    SetNativeCodeGenPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetNativeCodeGenPacket;
        this->bEnableNativeCodeGen = false;
    }

    static nlohmann::json Serialize(const SetNativeCodeGenPacket &packet) {
        return {{"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"enable_native_codegen", packet.bEnableNativeCodeGen}};
    }


    static SetNativeCodeGenPacket Deserialize(nlohmann::json json) {
        auto result = SetNativeCodeGenPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("enable_native_codegen").get_to(result.bEnableNativeCodeGen);

        return result;
    }
};
