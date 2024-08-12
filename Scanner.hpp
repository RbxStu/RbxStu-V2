//
// Created by Dottik on 10/8/2024.
//
#pragma once
#include <Windows.h>
#include <future>
#include <memory>
#include <string>
#include <vector>
#include "Logger.hpp"
#include "Utilities.hpp"

struct SignatureByte;
typedef std::vector<SignatureByte> Signature;

struct SignatureByte {
public:
    unsigned char szlookForByte;
    bool bIsWildcard;

    static Signature GetSignatureFromString(_In_ const std::string &aob, _In_ const std::string &mask) {
        auto logger = Logger::GetSingleton();
        auto nAob = Utilities::SplitBy(aob, ' ');
        auto nMask = Utilities::SplitBy(mask, ' ');
        if (nAob.size() != nMask.size()) {
            logger->PrintError(RbxStu::ByteScanner,
                               std::format("Failed to parse signature + mask. Reason: The array of bytes and the mask "
                                           "do not match in size when normalized. Size of AOB: {}. Size of Mask: {}",
                                           nAob.size(), nMask.size()));
            return {};
        }

        for (std::int32_t i = 0; i < nMask.size(); i++) {
            for (std::int32_t j = 0; j < nMask[i].size(); i++) {
                nMask[i][j] = std::tolower(nMask[i][j]); // Normalize mask to lower characters to avoid issues.
            }
        }

        Signature sig;
        for (std::int32_t i = 0; i < nAob.size(); i++) {
            if (nMask[i].find('x') != std::string::npos) {
                sig.push_back(SignatureByte{static_cast<unsigned char>('\0'), true});
                continue;
            }

            auto parsed = strtoul(nAob[i].data(), nullptr, 16);

            if (parsed > 255) { // We needn't bottom check, as its an unsigned long.
                logger->PrintWarning(
                        RbxStu::ByteScanner,
                        std::format(
                                "Failed to parse signature + mask. Reason: Value outside of the unsigned "
                                "char range. Value parsed: {}; Skipping byte, this may result in undefined behaviour.",
                                parsed));
                continue;
            }

            sig.push_back(SignatureByte{static_cast<unsigned char>(parsed), false});
        }
    }

    static Signature GetSignatureFromIDAString(_In_ const std::string &aob) {
        auto logger = Logger::GetSingleton();
        Signature sig;

        for (auto byte: Utilities::SplitBy(aob, ' ')) {
            if (byte.find('?') != std::string::npos) {
                sig.push_back(SignatureByte{static_cast<unsigned char>('\0'), true});
                continue;
            }

            auto parsed = strtoul(byte.data(), nullptr, 16);

            if (parsed > 255) { // We needn't bottom check, as its an unsigned long.
                logger->PrintWarning(
                        RbxStu::ByteScanner,
                        std::format(
                                "Failed to parse IDA signature. Reason: Value outside of the unsigned "
                                "char range. Value parsed: {}; Skipping byte, this may result in undefined behaviour.",
                                parsed));
                continue;
            }

            sig.push_back(SignatureByte{static_cast<unsigned char>(parsed), false});
        }

        return sig;
    }
};

/// @brief Allows you to do AOB Scans on the current process with a signature.
class Scanner final {
    static std::shared_ptr<Scanner> pInstance;

    /// @brief Matches the buffer contents against the signature.
    /// @param buffer [in] The buffer containing the chunk of memory to search at
    /// @param bufferSize [in] The size of the buffer.
    /// @param signature [in] Vector containing the SignatureBytes representing the signature in memory.
    /// @param memoryInformation [in] The basic memory information from which this information was grabbed. Required to
    /// calculate the base address of the returned void pointers.
    /// @return A vector containing all the addresses that matched, translated from the buffers address into the
    /// BaseAddress provided by memoryInformation.
    static std::vector<void *> ScanInternal(_In_ const unsigned char *buffer, _In_ const std::size_t bufferSize,
                                            _In_ const Signature &signature,
                                            _In_ const MEMORY_BASIC_INFORMATION &memoryInformation);

public:
    /// @brief Obtains the Singleton for the Scanner instance.
    /// @return Returns a shared pointer to the global Scanner singleton instance.
    static std::shared_ptr<Scanner> GetSingleton();

    /// @brief Scans from the given start address for the given signature.
    /// @param signature [in] A Vector containing the SignatureByte list that must be matched.
    /// @param lpStartAddress [in, opt] The address to start scanning from.
    std::vector<void *> Scan(_In_ const Signature &signature, _In_opt_ const void *lpStartAddress = nullptr);
};
