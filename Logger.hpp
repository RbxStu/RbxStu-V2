//
// Created by Dottik on 10/8/2024.
//
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Roblox/TypeDefinitions.hpp"

class Logger final {
    /// @brief Private, Static shared pointer into the instance.
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
    void Flush(RBX::Console::MessageType messageType);

    /// @brief Flushes the buffer only if the buffer is full.
    void FlushIfFull(RBX::Console::MessageType messageType);

public:
    /// @brief Obtains the Singleton for the Logger instance.
    /// @return Returns a shared pointer to the global Logger singleton instance.
    static std::shared_ptr<Logger> GetSingleton();

    void OpenStandard();
    /// @brief Initializes the Logger instance by opening the standard pipes, setting up the buffer and its size.
    /// @param bInstantFlush Whether the logger should keep no buffer, and let the underlying implementation for stdio
    /// and files handle it.
    void Initialize(bool bInstantFlush);
    void PrintDebug(const std::string &sectionName, const std::string &msg);

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


namespace RbxStu {
/// @brief Defines a section for use in the logger
#define DefineSectionName(varName, sectionName) constexpr auto varName = sectionName
    DefineSectionName(PacketSerdes, "RbxStu::PacketSerializer/Deserializer");
    DefineSectionName(Disassembler, "RbxStu::Disassembler");
    DefineSectionName(RobloxConsole, "RbxStu::RobloxConsole");
    DefineSectionName(ThreadManagement, "RbxStu::ThreadManagement");
    DefineSectionName(MainThread, "RbxStu::MainThread");
    DefineSectionName(ByteScanner, "RbxStu::ByteScanner");
    DefineSectionName(RobloxManager, "RbxStu::RobloxManager");
    DefineSectionName(LuauManager, "RbxStu::LuauManager");
    DefineSectionName(EnvironmentManager, "RbxStu::EnvironmentManager");
    DefineSectionName(Security, "RbxStu::Security");

    DefineSectionName(Anonymous, "RbxStu::Anonymous");

    DefineSectionName(HookedFunction, "RbxStu::HookedFunction<anonymous>");
    DefineSectionName(Hooked_RBXCrash, "RbxStu::HookedFunctions<RBX::RBXCRASH>");

    DefineSectionName(Scheduler, "RbxStu::Scheduler");
    DefineSectionName(Communication, "RbxStu::Communication");
    DefineSectionName(Env_Filesystem, "Env::Filesystem");

#undef DefineSectionName
}; // namespace RbxStu
