//
// Created by Dottik on 29/8/2024.
//

#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "CommunicationPacket.hpp"
#include "Packets/PacketBase.hpp"
#include "Utilities.hpp"

/*  Note on why this is here:
 *      - Packets are binary data, binary data comes in uint8_t (unsigned byte).
 *      - We can represent ANY "bigger" data type in simple bytes, we just need to extract it from the blob of bytes.
 *      - The following macros emit the code to do so, and It's fairly straight forward.
 *          - int32_t has 32 bits, a byte has 8 bits, this gives us that an int32_t is simply 4 bytes, which we can
 *            read, and place into an int32_t with bit-shifting.
 *          - This repeats with everything else. The only hindrance is reading floats, which simply put, are a pain, so,
 *            we just use a Quake III style cast (q_sqrt :heart:) to do our magic quickly without worrying about a
 *            mantissa, exponents and whatnot
 */

#define CanReadShort(input, atOffset) (((input.size() - atOffset) - sizeof(std::int16_t)) != 0)
#define ReadShort(input, offset) ({ static_cast<std::int16_t>((input.at(offset) << 8) | (input.at(offset + 1))); })

#define CanReadLong(input, atOffset) (((input.size() - atOffset) - sizeof(std::int32_t)) != 0)
#define ReadLong(input, offset)                                                                                        \
    static_cast<std::int32_t>(static_cast<std::int32_t>(input.at(offset)) << 24l) |                                    \
            (static_cast<std::int32_t>(input.at(offset + 1)) << 16l) |                                                 \
            (static_cast<std::int32_t>(input.at(offset + 2) << 8l) |                                                   \
             (static_cast<std::int32_t>(input.at(offset + 3))))


#define CanReadLongLong(input, atOffset) (((input.size() - atOffset) - sizeof(std::int64_t)) != 0)
#define ReadLongLong(input, offset)                                                                                    \
    static_cast<std::int64_t>((static_cast<std::int64_t>(input.at(offset)) << 56ll) |                                  \
                              (static_cast<std::int64_t>(input.at(offset + 1)) << 48ll) |                              \
                              (static_cast<std::int64_t>(input.at(offset + 2)) << 40ll) |                              \
                              (static_cast<std::int64_t>(input.at(offset + 3)) << 32ll) |                              \
                              (static_cast<std::int64_t>(input.at(offset + 4)) << 24ll) |                              \
                              (static_cast<std::int64_t>(input.at(offset + 5)) << 16ll) |                              \
                              (static_cast<std::int64_t>(input.at(offset + 6)) << 8ll) |                               \
                              (static_cast<std::int64_t>(input.at(offset + 7))))

#define CanReadFloat CanReadLong // Floats are 32 bytes anyway.
#define ReadFloat(input, offset) *reinterpret_cast<float *>(&ReadLong(input, offset)) // Quake III type shit.

#define CanReadDouble CanReadLongLong // Doubles are 64 bytes anyway.
#define ReadDouble(input, offset) *reinterpret_cast<double *>(&ReadLongLong(input, offset)) // Quake III type shit.


class PacketSerdes final {
    static std::shared_ptr<PacketSerdes> pInstance;

public:
    PacketSerdes() = default;

    static std::shared_ptr<PacketSerdes> GetSingleton();

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    __inline std::optional<CommunicationPacket<T>> DeserializeFromString(const std::string &input) {
        return this->DeserializeFromBytes<T>(std::vector<std::uint8_t>{input.begin(), input.end()});
    }

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    __inline std::optional<CommunicationPacket<T>> DeserializeFromBytes(const std::vector<std::uint8_t> &input) {
        const auto logger = Logger::GetSingleton();

        if (!CanReadLong(input, 0)) {
            logger->PrintError(RbxStu::PacketSerdes, "Malformed packet received: Packet ID cannot be read.");
            return {};
        }
        const std::uint32_t packetId = ReadLong(input, 0);
        if (!CanReadLongLong(input, 4)) {
            logger->PrintError(RbxStu::PacketSerdes, "Malformed packet received: Packet Size cannot be read.");
            return {};
        }
        const std::uint64_t packetSize = ReadLongLong(input, 4);

        // packetSize should not include packetId nor packetSize, only packetData.
        if ((packetSize + sizeof(std::uint64_t) + sizeof(std::int32_t)) != input.size()) {
            logger->PrintError(RbxStu::PacketSerdes, "Malformed packet received: Packet Size appears to be too small, "
                                                     "refusing to read malformed packet.");
            return {};
        }

        if (packetSize != sizeof(T)) {
            logger->PrintError(RbxStu::PacketSerdes,
                               "Malformed packet received: Packet Size is not equal to the size of the structure T!");
            return {};
        }

        const auto dataPointer =
                reinterpret_cast<T *>(reinterpret_cast<std::uintptr_t>(const_cast<std::uint8_t *>(input.data())) +
                                      sizeof(std::uint64_t) + sizeof(std::int32_t));

        if (sizeof(T) != (input.size() - sizeof(std::uint64_t) - sizeof(std::int32_t))) {
            logger->PrintError(
                    RbxStu::PacketSerdes,
                    std::format(
                            "Malformed packet received: sizeof(T) != data.size()! PacketID: {:#x}, PacketSize: {:#x}",
                            packetId, packetSize));
        }

        auto packet = CommunicationPacket<T>{0};
        packet.ulPacketId = packetId;
        packet.ullPacketSize = packetSize;
        packet.vData = std::move(dataPointer);

        return packet;
    }

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    __inline std::vector<std::uint8_t> SerializeFromStructure(const T &serialize) {
        auto bytes = std::bit_cast<std::uint8_t *>(&serialize);

        auto vec = std::vector<std::uint8_t>(bytes, bytes + (sizeof(T) / sizeof(std::uint8_t)));
        vec.push_back(static_cast<std::uint32_t>(serialize.ulPacketId));
        vec.push_back(static_cast<std::uint64_t>(sizeof(T)));
        for (int i = 0; i < sizeof(T) / sizeof(std::uint8_t); ++i) {
            auto pByte = bytes++;
            vec.push_back(*pByte);
        }

        return vec;
    }

    template<RbxStu::Concepts::TypeConstraint<PacketBase> T>
    __forceinline static bool ContainsFlag(CommunicationPacket<T> packet, std::uint64_t flag) {
        return (packet.vData->ullPacketFlags & flag) == flag;
    }

    __forceinline std::int32_t ReadPacketIdentifier(const std::string &str) {
        return ReadPacketIdentifier(std::vector<std::uint8_t>{str.begin(), str.end()});
    }

    __forceinline std::int32_t ReadPacketIdentifier(const std::vector<std::uint8_t> &data) {
        if (CanReadLong(data, 0))
            return ReadLong(data, 0);
        else
            return -1;
    }
};


#undef CanReadLong
#undef ReadLong

#undef CanReadLongLong
#undef ReadLongLong

#undef CanReadFloat
#undef ReadFloat

#undef CanReadDouble
#undef ReadDouble
