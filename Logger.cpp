//
// Created by Dottik on 10/8/2024.
//

#include "Logger.hpp"

#include <format>
#include <iostream>

std::shared_ptr<Logger> Logger::pInstance; // Static var.

std::shared_ptr<Logger> Logger::GetSingleton() {
    if (Logger::pInstance == nullptr)
        Logger::pInstance = std::make_shared<Logger>();

    return Logger::pInstance;
}


void Logger::Flush() {
    // TODO: Implement flushing to file.
    std::cout << this->m_szMessageBuffer << std::endl;
    this->m_szMessageBuffer.clear();
}

void Logger::FlushIfFull() {
    if (!this->m_bInitialized)
        throw std::exception(
                std::format("The logger instance @ {} is not initialized!", reinterpret_cast<uintptr_t>(this)).c_str());

    if (this->m_szMessageBuffer.length() >= this->m_dwBufferSize) {
        this->Flush();
    }
}

void Logger::Initialize() {
    if (this->m_bInitialized)
        return;

    freopen_s(nullptr, "CONOUT$", "w", stdout);
    freopen_s(nullptr, "CONIN$", "r", stdin);
    freopen_s(nullptr, "CONERR$", "w", stderr);
    this->m_dwBufferSize = 0xffff;
    this->m_szMessageBuffer = std::string("");
    this->m_szMessageBuffer.reserve(this->m_dwBufferSize);
    this->m_bInitialized = true;
    std::atexit([] {
        const auto logger = Logger::GetSingleton();
        logger->Flush();
        logger->m_szMessageBuffer.clear();
    });
}

void Logger::PrintInformation(const std::string &sectionName, const std::string &msg) {
    this->m_szMessageBuffer.append(std::format("[INFO/{}] {}", sectionName, msg));
    this->FlushIfFull();
}

void Logger::PrintWarning(const std::string &sectionName, const std::string &msg) {
    this->m_szMessageBuffer.append(std::format("[WARN/{}] {}", sectionName, msg));
    this->FlushIfFull();
}

void Logger::PrintError(const std::string &sectionName, const std::string &msg) {
    this->m_szMessageBuffer.append(std::format("[ERROR/{}] {}", sectionName, msg));
    this->FlushIfFull();
}
