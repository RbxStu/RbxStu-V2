//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>


#include "PacketBase.hpp"

enum SetFastVariablePacketFlag { Boolean = 0b0, Integer = 0b1, String = 0b10, Unassigned = 0b100 };

class SetFastVariablePacket final : public PacketBase {
public:
    std::string szNewValue;
    int lNewValue;
    bool bNewValue;
    std::string szFastVariableName;

    __forceinline SetFastVariablePacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::SetFastVariablePacket;
        this->ullPacketFlags = SetFastVariablePacketFlag::Unassigned;
    }

    static nlohmann::json Serialize(const SetFastVariablePacket &packet) {
        nlohmann::json json = {
                {"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"fast_variable_name", packet.szFastVariableName},
        };

        switch (packet.ullPacketFlags) {
            case SetFastVariablePacketFlag::Boolean:
                json["fast_variable"] = packet.bNewValue;
                break;
            case SetFastVariablePacketFlag::Integer:
                json["fast_variable"] = packet.lNewValue;
                break;
            case SetFastVariablePacketFlag::String:
                json["fast_variable"] = packet.szNewValue;
                break;
            case SetFastVariablePacketFlag::Unassigned:
            default:
                json["fast_variable"] = "???";
                break;
        }

        return json;
    }


    static SetFastVariablePacket Deserialize(const nlohmann::json &json) {
        auto result = SetFastVariablePacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("fast_variable_name").get_to(result.szFastVariableName);
        switch (result.ullPacketFlags) {
            case SetFastVariablePacketFlag::Boolean:
                json.at("fast_variable").get_to(result.bNewValue);
                break;
            case SetFastVariablePacketFlag::Integer:
                json.at("fast_variable").get_to(result.lNewValue);
                break;
            case SetFastVariablePacketFlag::String:
                json.at("fast_variable").get_to(result.szNewValue);
                break;
            case SetFastVariablePacketFlag::Unassigned:
            default:
                result.szNewValue = "???";
                result.lNewValue = -1;
                result.bNewValue = false;
                break;
        }

        return result;
    }
};
