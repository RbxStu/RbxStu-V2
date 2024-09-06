//
// Created by Dottik on 30/8/2024.
//

#pragma once
#include <cstdint>

#include "Utilities.hpp"

namespace RbxStu::WebSocketCommunication {
    /// @brief Used to document and keep track of RbxStu V2 packets
    enum PacketIdentifier : std::uint32_t {
        /// @brief Sends the settings of RbxStu V2, sent when the communication begins.
        /// @remarks This packet will not always return the default data, if, for example, it were to reconnect to the
        /// UI.
        HelloPacket = 0x0,

        /// @brief Sets the value of a Fast Variable to the provided value.
        /// @remarks Depends on ullPacketFlags to declare the type that will be read from the union.
        SetFastVariablePacket = 0x1,

        /// @brief Sets the status of a function to mandate if it is blocked or not.
        SetFunctionBlockStatePacket = 0x2,

        /// @brief Sets whether RbxStu V2 will emit Luau code that will be compiled to native code, this may result
        /// in increased memory usage.
        SetNativeCodeGenPacket = 0x3,

        /// @brief Sets the Fingerprint used on the HTTP header for game:HttpGet, httpget, and
        /// request/http_request/http.request.
        SetRequestFingerprintPacket = 0x4,

        /// @brief Sets the safe mode for RbxStu V2. This will toggle all built-in security, beware.
        SetSafeModePacket = 0x5,

        /// @brief Sets the DataModel used for execution.
        /// @remarks This packet uses PacketFlags.
        SetExecutionDataModelPacket = 0x6,

        /// @brief Returned by every WebSocket protocol operation by RbxStu V2.
        /// @remarks This packet uses PacketFlags.
        ResponseStatusPacket = 0x7,

        /// @brief Schedules the given Luau code into the RbxStu V2 scheduler for execution.
        ScheduleLuauPacket = 0x8,

        /// @brief Returned after a Scheduler job is completed.
        ScheduleLuauResponsePacket = 0x9,

        /// @brief Allows to toggle access to the .Source property of LuaSourceContainer inheritors
        /// (LocalScript, Script, ModuleScript, ...)
        /// @remarks This packet uses PacketFlags.
        SetScriptSourceAccessPacket = 0xA,
    };
}; // namespace RbxStu::WebSocketCommunication

class PacketFunctions abstract {
public:
    static nlohmann::json Serialize(const PacketFunctions &packet);
    static PacketFunctions Deserialize(const nlohmann::json &json);
};

class PacketBase : public PacketFunctions {
public:
    virtual ~PacketBase() = default;

    RbxStu::WebSocketCommunication::PacketIdentifier ulPacketId;
    std::uint64_t ullPacketFlags{0};

    PacketBase() {
        ulPacketId = static_cast<RbxStu::WebSocketCommunication::PacketIdentifier>(0);
        ullPacketFlags = 0;
    }

    static nlohmann::json Serialize(const PacketBase &packet) {
        return {{"packet_id", packet.ulPacketId}, {"packet_flags", packet.ullPacketFlags}};
    }


    static PacketBase Deserialize(nlohmann::json json) {
        auto result = PacketBase{};

        json.at("packet_id").get_to(result.ulPacketId);
        json.at("packet_flags").get_to(result.ullPacketFlags);

        return result;
    }
};
