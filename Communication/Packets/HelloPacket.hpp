//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>


#include "PacketBase.hpp"

struct HelloPacket final : public PacketBase {
    bool bIsNativeCodeGenEnabled;
    bool bIsSafeModeEnabled;
    std::int32_t lCurrentExecutionDataModel;
    char szFingerprintHeader[0xFF];

    __forceinline HelloPacket() {
        this->ulPacketId = RbxStu::WebSocketCommunication::HelloPacket;
        this->bIsNativeCodeGenEnabled = false;
        this->bIsSafeModeEnabled = false;
        this->lCurrentExecutionDataModel = 0;
        memset(this->szFingerprintHeader, '1', sizeof(this->szFingerprintHeader));
        this->szFingerprintHeader[0xFE] = 0;
    }
};
