//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>


#include "PacketBase.hpp"

class HelloPacket final : public PacketBase {
public:
    bool bIsNativeCodeGenEnabled;
    bool bIsSafeModeEnabled;
    std::int32_t lCurrentExecutionDataModel;
    std::string szFingerprintHeader;

    __forceinline HelloPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::HelloPacket;
        this->bIsNativeCodeGenEnabled = false;
        this->bIsSafeModeEnabled = false;
        this->lCurrentExecutionDataModel = 0;
    }

    static nlohmann::json Serialize(const HelloPacket &packet) {
        return {{"packet_id", packet.ulPacketId},
                {"packet_flags", packet.ullPacketFlags},
                {"is_safe_mode_enabled", packet.bIsSafeModeEnabled},
                {"is_codegen_enabled", packet.bIsNativeCodeGenEnabled},
                {"current_execution_datamodel", packet.lCurrentExecutionDataModel},
                {"http_fingerprint_header", packet.szFingerprintHeader}};
    }

    static HelloPacket Deserialize(nlohmann::json json) {
        auto result = HelloPacket{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);
        json.at("is_safe_mode_enabled").get_to(result.bIsSafeModeEnabled);
        json.at("is_codegen_enabled").get_to(result.bIsNativeCodeGenEnabled);
        json.at("current_execution_datamodel").get_to(result.lCurrentExecutionDataModel);
        json.at("http_fingerprint_header").get_to(result.szFingerprintHeader);

        return result;
    }
};
