//
// Created by Yoru on 8/16/2024.
//

#include "Communication.hpp"
#include <Windows.h>
#include "Logger.hpp"
#include "Scheduler.hpp"

#include "ixwebsocket/IXWebSocket.h"

std::shared_ptr<Communication> Communication::pInstance;

static std::mutex __get_singleton_lock{};

std::shared_ptr<Communication> Communication::GetSingleton() {
    std::scoped_lock lock{__get_singleton_lock};
    if (Communication::pInstance == nullptr)
        Communication::pInstance = std::make_shared<Communication>();

    return Communication::pInstance;
}

bool Communication::IsUnsafeMode() const { return this->m_bIsUnsafe; }

void Communication::SetUnsafeMode(bool isUnsafe) {
    this->m_bIsUnsafe = isUnsafe;
    Logger::GetSingleton()->PrintWarning(
            RbxStu::Communication, "WARNING! UNSAFE MODE IS ENABLED! THIS CAN HAVE CONSEQUENCES THAT GO HIGHER THAN "
                                   "YOU THINK! IF YOU DO NOT KNOW WHAT YOU'RE DOING, TURN SAFE MODE BACK ON!");
}
bool Communication::IsCodeGenerationEnabled() const { return this->m_bEnableCodeGen; }
void Communication::SetCodeGenerationEnabled(bool enableCodeGen) { this->m_bEnableCodeGen = enableCodeGen; }

void Communication::NewCommunication() {
    const auto logger = Logger::GetSingleton();
    const auto scheduler = Scheduler::GetSingleton();

    logger->PrintInformation(RbxStu::Communication, "Starting ix::WebSocketServer for communication!");
    const auto webSocket = std::make_unique<ix::WebSocket>();
    webSocket->start();
}

void Communication::HandlePipe(const std::string &szPipeName) {
    const auto logger = Logger::GetSingleton();
    const auto scheduler = Scheduler::GetSingleton();
    DWORD Read{};
    char BufferSize[999999];
    std::string name = (R"(\\.\pipe\)" + szPipeName), Script{};
    HANDLE hPipe = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, PIPE_WAIT,
                                    1, 9999999, 9999999, NMPWAIT_USE_DEFAULT_WAIT, nullptr);

    logger->PrintInformation(RbxStu::Communication,
                             std::format("Created Named ANSI Pipe -> '{}' for obtaining Luau code.", name));

    logger->PrintInformation(RbxStu::Communication, "Connecting to Named Pipe...");
    ConnectNamedPipe(hPipe, nullptr);

    logger->PrintInformation(RbxStu::Communication, "Connected! Awaiting Luau code!");
    while (hPipe != INVALID_HANDLE_VALUE && ReadFile(hPipe, BufferSize, sizeof(BufferSize) - 1, &Read, nullptr)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            logger->PrintError(RbxStu::Communication, "RbxStu does not handle asynchronous IO requests. Pipe "
                                                      "requests must be synchronous (This should not happen)");
            _mm_pause();
            continue;
        }
        BufferSize[Read] = '\0';
        Script += BufferSize;

        if (scheduler->IsInitialized()) {
            logger->PrintInformation(RbxStu::Communication, "Luau code scheduled for execution.");
            scheduler->ScheduleJob(SchedulerJob(Script));

        } else {
            logger->PrintWarning(RbxStu::Communication,
                                 "RbxStu::Scheduler is NOT initialized! Luau code NOT scheduled, please enter into a "
                                 "game to be able to enqueue jobs into the Scheduler for execution!");
        }
        Script.clear();
    }
}
