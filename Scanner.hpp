//
// Created by Dottik on 10/8/2024.
//
#pragma once
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

    static Signature GetSignatureFromString(const std::string &aob, const std::string &mask) {
        // TODO: Implement signature parser.
    }

    static Signature GetSignatureFromIDAString(const std::string &aob) {
        auto logger = Logger::GetSingleton();
        Signature sig;

        for (auto byte: Utilities::SplitBy(aob, ' ')) {
            if (byte.find('?') != std::string::npos) {
                sig.push_back(SignatureByte{static_cast<unsigned char>('\0'), true});
                continue;
            }

            auto parsed = strtoul(byte.data(), nullptr, 16);

            if (parsed > 255) { // We needn't bottom check, as its an unsigned long.
                logger->PrintWarning(RbxStu::ByteScanner,
                                     std::format("Failed to parse IDA signature. Reason: Value outside of the unsigned "
                                                 "char range. Value parsed: {}; Skipping byte, this may result in undefined behaviour.",
                                                 parsed));
                continue;
            }

            sig.push_back(SignatureByte{static_cast<unsigned char>(parsed), false});
        }

        return sig;
    }
};


class Scanner final {
    static std::shared_ptr<Scanner> pInstance;

public:
    /// @brief Obtains the Singleton for the Scanner instance.
    /// @return Returns a shared pointer to the global Scanner singleton instance.
    static std::shared_ptr<Scanner> GetSingleton();

    std::vector<void *> Scan(const Signature &signature, void *lpStartAddress) {}
};
