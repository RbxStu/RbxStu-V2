//
// Created by Yoru on 8/16/2024.
//

#include "Communication.hpp"
#include <Windows.h>
#include "Logger.hpp"
#include "Scheduler.hpp"

std::shared_ptr<Communication> Communication::pInstance;

std::shared_ptr<Communication> Communication::GetSingleton() {
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

        logger->PrintInformation(RbxStu::Communication, "Pipe request received! Scheduling...");
        scheduler->ScheduleJob(SchedulerJob(Script));
        Script.clear();
    }
}
