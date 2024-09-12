//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstring>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "PacketBase.hpp"

enum DataModelUpdatePacketFlags {
    EditDataModel = 0b1,
    ClientDataModel = 0b10,
    ServerDataModel = 0b100,
    StandaloneDataModel = 0b1000,

    Created = 0b10000,
    Destroyed = 0b100000,
};

class DataModelUpdatePacket final : public PacketBase {
public:
    __forceinline DataModelUpdatePacket(RBX::DataModelType dataModel, bool wasCreated = false) {
        this->ulPacketId = RbxStu::WebSocketCommunication::DataModelUpdatePacket;
        this->ullPacketFlags =
                (1ull << static_cast<std::uint64_t>(dataModel)) | (wasCreated ? (1ull << 5ull) : (1ull << 6ull));
    }

    static nlohmann::json Serialize(const DataModelUpdatePacket &packet) {
        return {{"packet_id", packet.ulPacketId}, {"packet_flags", packet.ullPacketFlags}};
    }

    static DataModelUpdatePacket Deserialize(nlohmann::json json) {
        auto result = DataModelUpdatePacket{RBX::DataModelType::DataModelType_Edit, false};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);

        return result;
    }
};
