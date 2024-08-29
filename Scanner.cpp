//
// Created by Dottik on 10/8/2024.
//

#include "Scanner.hpp"

std::shared_ptr<Scanner> Scanner::pInstance;

Signature SignatureByte::GetSignatureFromString(const std::string &aob, const std::string &mask) {
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

        if (parsed > 255) { // We needn't bottom check, as it's an unsigned long.
            logger->PrintWarning(
                    RbxStu::ByteScanner,
                    std::format("Failed to parse signature + mask. Reason: Value outside of the unsigned "
                                "char range. Value parsed: {}; Skipping byte, this may result in undefined behaviour.",
                                parsed));
            continue;
        }

        sig.push_back(SignatureByte{static_cast<unsigned char>(parsed), false});
    }

    return sig;
}
Signature SignatureByte::GetSignatureFromIDAString(const std::string &aob) {
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
                    std::format("Failed to parse IDA signature. Reason: Value outside of the unsigned "
                                "char range. Value parsed: {}; Skipping byte, this may result in undefined behaviour.",
                                parsed));
            continue;
        }

        sig.push_back(SignatureByte{static_cast<unsigned char>(parsed), false});
    }

    return sig;
}


std::vector<void *> Scanner::ScanInternal(const unsigned char *buffer, const std::size_t bufferSize,
                                          const Signature &signature,
                                          const MEMORY_BASIC_INFORMATION &memoryInformation) {
    auto signatureLength = signature.size();
    std::vector<void *> vec{};
    for (std::uint32_t i = 0; i < bufferSize - signatureLength; i++) {
        bool success = true;
        auto offsetBuffer = reinterpret_cast<const char *>(reinterpret_cast<std::uintptr_t>(buffer) + i);

        for (std::uint32_t j = 0; j < signatureLength; j++) {
            if (!signature[j].bIsWildcard &&
                signature[j].szlookForByte !=
                        *reinterpret_cast<const unsigned char *>(reinterpret_cast<std::uintptr_t>(offsetBuffer) + j)) {
                success = false; // Byte is not the same.
                break;
            }
        }
        if (success) {
            vec.push_back(
                    reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(memoryInformation.BaseAddress) + i));
        }
    }
    Sleep(1);
    return vec;
}
std::shared_ptr<Scanner> Scanner::GetSingleton() {
    if (Scanner::pInstance == nullptr)
        Scanner::pInstance = std::make_shared<Scanner>();

    return Scanner::pInstance;
}
std::vector<void *> Scanner::Scan(const Signature &signature, const void *lpStartAddress) {
    const auto logger = Logger::GetSingleton();

    if (lpStartAddress == nullptr) {
        logger->PrintWarning(RbxStu::ByteScanner,
                             "lpStartAddress was nullptr. Assuming the intent of the caller was for "
                             "lpStartAddress to be equal to GetModuleHandle(nullptr).");
        lpStartAddress = reinterpret_cast<void *>(GetModuleHandle(nullptr));
    }

    std::vector<std::future<std::vector<void *>>> scansVector{};
    std::vector<void *> results{};
    MEMORY_BASIC_INFORMATION memoryInfo{};
    logger->PrintDebug(RbxStu::ByteScanner,
                       std::format("Beginning scan from address {} to far beyond!", lpStartAddress));
    auto startAddress = reinterpret_cast<std::uintptr_t>(lpStartAddress);

    while (VirtualQuery(reinterpret_cast<void *>(startAddress), &memoryInfo, sizeof(MEMORY_BASIC_INFORMATION))) {
        scansVector.push_back(std::async(std::launch::async, [memoryInfo, &signature]() {
            bool valid = memoryInfo.State == MEM_COMMIT;
            valid &= (memoryInfo.Protect & PAGE_GUARD) == 0;
            valid &= (memoryInfo.Protect & PAGE_NOACCESS) == 0;
            valid &= (memoryInfo.Protect & PAGE_READWRITE) == 0;
            valid &= (memoryInfo.Protect & PAGE_READONLY) == 0;
            valid &= (memoryInfo.Protect & PAGE_EXECUTE_WRITECOPY) == 0;
            valid &= (memoryInfo.Protect & PAGE_WRITECOPY) == 0;
            valid &= memoryInfo.Type == MEM_PRIVATE || memoryInfo.Type == MEM_IMAGE;

            if (!valid) {
                const auto logger = Logger::GetSingleton();
                logger->PrintDebug(
                        RbxStu::ByteScanner,
                        std::format("Memory block at address {} is invalid for scanning.", memoryInfo.BaseAddress));
                return std::vector<void *>{};
            }
            auto *buffer = new unsigned char[memoryInfo.RegionSize];
            memcpy(buffer, memoryInfo.BaseAddress, memoryInfo.RegionSize);

            auto scanResult = Scanner::ScanInternal(buffer, memoryInfo.RegionSize, signature, memoryInfo);
            delete[] buffer;
            return scanResult;
        }));

        startAddress += memoryInfo.RegionSize;
    }

    for (auto &i: scansVector) {
        auto async_result = i.get();
        for (const auto &e: async_result) {
            results.push_back(e);
        }
    }

    logger->PrintDebug(RbxStu::ByteScanner,
                       std::format("Scan finalized. Found {} candidates for the given signature.", results.size()));
    return results;
}
