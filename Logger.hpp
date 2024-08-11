//
// Created by Dottik on 10/8/2024.
//
#pragma once

#include <cstdint>
#include <memory>
#include <string>

class Logger final {
    static std::shared_ptr<Logger> pInstance;

    /// @brief Disables buffering.
    bool m_bInstantFlush;
    /// @brief Defines whether the Logger instance is initialized or not.
    bool m_bInitialized;
    /// @brief The size of the buffer.
    std::uint32_t m_dwBufferSize;
    /// @brief The buffer used to store messages..
    std::string m_szMessageBuffer;

    /// @brief Flushes the buffer into the standard output.
    void Flush();

    /// @brief Flushes the buffer only if the buffer is full.
    void FlushIfFull();

public:
    /// @brief Obtains the Singleton for the Logger instance.
    /// @return Returns a shared pointer to the global Logger singleton instance.
    static std::shared_ptr<Logger> GetSingleton();

    /// @brief Initializes the Logger instance by opening the standard pipes, setting up the buffer and its size.
    /// @param bInstantFlush Whether the logger should keep no buffer, and let the underlying implementation for stdio
    /// and files handle it.
    void Initialize(bool bInstantFlush);

    /// @brief Emits an Information with the given section name into the Logger's buffer.
    /// @param sectionName The name of the section that the code is running at
    /// @param msg The content to write into the buffer, as an information.
    void PrintInformation(const std::string &sectionName, const std::string &msg);

    /// @brief Emits a Warning with the given section name into the Logger's buffer.
    /// @param sectionName The name of the section that the code is running at
    /// @param msg The content to write into the buffer, as a warning.
    void PrintWarning(const std::string &sectionName, const std::string &msg);

    /// @brief Emits an error with the given section name into the Logger's buffer.
    /// @param sectionName The name of the section that the code is running at
    /// @param msg The content to write into the buffer, as an error.
    void PrintError(const std::string &sectionName, const std::string &msg);
};

/// @brief Defines a section for use in the logger
#define DefineSectionName(varName, sectionName) const auto varName = sectionName

namespace RbxStu {
    DefineSectionName(Execution, "RbxStu::Execution");
    DefineSectionName(MainThread, "RbxStu::MainThread");
    DefineSectionName(ByteScanner, "RbxStu::ByteScanner");
}; // namespace RbxStu
