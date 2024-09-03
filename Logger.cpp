//
// Created by Dottik on 10/8/2024.
//

#include "Logger.hpp"

#include <Termcolor.hpp>
#include <format>
#include <iostream>
#include <mutex>
#include <shared_mutex>

#include "Roblox/TypeDefinitions.hpp"

std::shared_mutex mutex;
std::shared_ptr<Logger> Logger::pInstance; // Static var.

std::shared_ptr<Logger> Logger::GetSingleton() {
    if (Logger::pInstance == nullptr)
        Logger::pInstance = std::make_shared<Logger>();

    return Logger::pInstance;
}
void Logger::OpenStandard() {
    freopen_s(reinterpret_cast<FILE **>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE **>(stdin), "CONIN$", "r", stdin);
    freopen_s(reinterpret_cast<FILE **>(stderr), "CONOUT$", "w", stderr);
}

void Logger::Flush(const RBX::Console::MessageType messageType) {
    // TODO: Implement flushing to file.
    switch (messageType) {
        case RBX::Console::MessageType::Error:
            std::cout << termcolor::bright_red << this->m_szMessageBuffer << termcolor::reset << std::endl;
            break;
        case RBX::Console::MessageType::InformationBlue:
            std::cout << termcolor::bright_blue << this->m_szMessageBuffer << termcolor::reset << std::endl;
            break;
        case RBX::Console::MessageType::Warning:
            std::cout << termcolor::bright_yellow << this->m_szMessageBuffer << termcolor::reset << std::endl;
            break;
        case RBX::Console::MessageType::Standard:
            std::cout << this->m_szMessageBuffer << std::endl;
            break;
    }
    this->m_szMessageBuffer.clear();
}

void Logger::FlushIfFull(const RBX::Console::MessageType messageType) {
    if (!this->m_bInitialized)
        throw std::exception(
                std::format("The logger instance @ {} is not initialized!", reinterpret_cast<uintptr_t>(this)).c_str());

    if (this->m_bInstantFlush || this->m_szMessageBuffer.length() >= this->m_dwBufferSize)
        this->Flush(messageType);
}

void Logger::Initialize(const bool bInstantFlush) {
    if (this->m_bInitialized)
        return;

    this->OpenStandard();
    this->m_dwBufferSize = 0xffff;
    this->m_szMessageBuffer = std::string("");
    this->m_szMessageBuffer.reserve(this->m_dwBufferSize);
    this->m_bInstantFlush = bInstantFlush;
    this->m_bInitialized = true;
    std::atexit([] {
        const auto logger = Logger::GetSingleton();
        logger->Flush(RBX::Console::Standard);
        logger->m_szMessageBuffer.clear();
    });
}

void Logger::PrintDebug(const std::string &sectionName, const std::string &msg) {
#if _DEBUG
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[DEBUG/{}] {}", sectionName, msg));
    this->FlushIfFull(RBX::Console::InformationBlue);
#endif
}

void Logger::PrintInformation(const std::string &sectionName, const std::string &msg) {
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[INFO/{}] {}", sectionName, msg));
    this->FlushIfFull(RBX::Console::InformationBlue);
}

void Logger::PrintWarning(const std::string &sectionName, const std::string &msg) {
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[WARN/{}] {}", sectionName, msg));
    this->FlushIfFull(RBX::Console::Warning);
}

void Logger::PrintError(const std::string &sectionName, const std::string &msg) {
    std::lock_guard lock{mutex};
    this->m_szMessageBuffer.append(std::format("[ERROR/{}] {}", sectionName, msg));
    this->FlushIfFull(RBX::Console::Error);
}
