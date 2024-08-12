//
// Created by Dottik on 10/8/2024.
//

#include "Scanner.hpp"

std::shared_ptr<Scanner> Scanner::pInstance;

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
        logger->PrintWarning(RbxStu::ByteScanner, "lpStartAddress was nullptr. Assuming the intent of the caller was for "
                                                  "lpStartAddress to be equal to GetModuleHandle(nullptr).");
        lpStartAddress = reinterpret_cast<void *>(GetModuleHandle(nullptr));
    }

    std::vector<std::future<std::vector<void *>>> scansVector{};
    std::vector<void *> results{};
    MEMORY_BASIC_INFORMATION memoryInfo{};
    logger->PrintInformation(RbxStu::ByteScanner,
                             std::format("Beginning scan from address {} to far beyond!", lpStartAddress));

    auto startAddress = reinterpret_cast<std::uintptr_t>(lpStartAddress);

    while (VirtualQuery(reinterpret_cast<void *>(startAddress), &memoryInfo, sizeof(MEMORY_BASIC_INFORMATION))) {
        scansVector.push_back(std::async(std::launch::async, [memoryInfo, &signature]() {
            bool valid = memoryInfo.State == MEM_COMMIT;
            valid &= (memoryInfo.Protect & PAGE_GUARD) == 0;
            valid &= (memoryInfo.Protect & PAGE_NOACCESS) == 0;
            valid &= memoryInfo.Type == MEM_PRIVATE || memoryInfo.Type == MEM_IMAGE;

            if (!valid) {
                // logger->PrintInformation(
                //         RbxStu::ByteScanner,
                //         std::format("Memory block at address {} is invalid for scanning.",
                //         memoryInfo.BaseAddress));
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

    logger->PrintInformation(
            RbxStu::ByteScanner,
            std::format("Scan finalized. Found {} candidates for the given signature.", results.size()));
    return results;
}
