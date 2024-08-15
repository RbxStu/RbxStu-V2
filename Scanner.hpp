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
    unsigned char szlookForByte;
    bool bIsWildcard;

    static Signature GetSignatureFromString(_In_ const std::string &aob, _In_ const std::string &mask);

    static Signature GetSignatureFromIDAString(_In_ const std::string &aob);
};

/// @brief Allows you to do AOB Scans on the current process with a signature.
class Scanner final {
    /// @brief Private, Static shared pointer into the instance.
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
    /// @return A std::vector<void *> containing the start of any matched memory blocks.
    /// @remarks Scan will skip non-executable segments in an effort to increase scanning speed. This leads to .rdata
    /// and .data being not able to be sigged, whilst the need for such behaviour is rather rare, and is a reason why it
    /// is not supported.
    std::vector<void *> Scan(_In_ const Signature &signature,
                             _In_opt_ const void *lpStartAddress = GetModuleHandle(nullptr));
};
