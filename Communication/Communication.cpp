//
// Created by Yoru on 8/16/2024.
//

#include "Communication.hpp"

#include <HttpStatus.hpp>
#include <Windows.h>
#include "Logger.hpp"
#include "PacketSerdes.hpp"
#include "Packets/HelloPacket.hpp"
#include "Packets/ResponseStatusPacket.hpp"
#include "Packets/SetNativeCodeGenPacket.hpp"
#include "Packets/SetRequestFingerprintPacket.hpp"
#include "Packets/SetSafeModePacket.hpp"
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
std::string Communication::SetFingerprintHeader(const std::string &header) {
    if (header.find("-Fingerprint") == std::string::npos)
        return this->m_szFingerprintHeader;

    auto old = this->m_szFingerprintHeader;
    this->m_szFingerprintHeader = header;
    return old;
}
const std::string &Communication::GetFingerprintHeaderName() { return this->m_szFingerprintHeader; }
bool Communication::IsCodeGenerationEnabled() const { return this->m_bEnableCodeGen; }
void Communication::SetCodeGenerationEnabled(bool enableCodeGen) { this->m_bEnableCodeGen = enableCodeGen; }

[[noreturn]] void Communication::NewCommunication(const std::string &szRemoteHost) {
    const auto logger = Logger::GetSingleton();
    const auto scheduler = Scheduler::GetSingleton();
    const auto serializer = PacketSerdes::GetSingleton();
    const auto communication = Communication::GetSingleton();

    if (szRemoteHost.find("ws://") == std::string::npos && szRemoteHost.find("wss://") == std::string::npos) {
        logger->PrintError(RbxStu::Communication, "Cannot begin ix::WebSocket! ");
        throw std::exception("szRemoteHost does not follow the protocol of ws:// or wss://! Cowardly refusing to begin "
                             "communication.");
    }

    logger->PrintInformation(RbxStu::Communication, "Starting ix::WebSocket for communication!");
    const auto webSocket = std::make_unique<ix::WebSocket>();
    webSocket->setUrl(szRemoteHost);
    webSocket->setOnMessageCallback([&communication, &serializer, &logger,
                                     &webSocket](const ix::WebSocketMessagePtr &message) {
        switch (message->type) {
            case ix::WebSocketMessageType::Message: {
                if (!message->binary) {
                    logger->PrintWarning(RbxStu::Communication, "RbxStu only supports binary inputs. Packet dropped.");
                    return;
                }

                auto responseStatus = ResponseStatusPacket{};
                responseStatus.ullPacketFlags = ResponseStatusPacketFlags::Success;

                auto sendResponseStatus = true;
                auto wasSuccess = false;

                const auto packetId = static_cast<RbxStu::WebSocketCommunication::PacketIdentifier>(
                        serializer->ReadPacketIdentifier(message->str));

                switch (packetId) {
                    case RbxStu::WebSocketCommunication::HelloPacket: {
                        logger->PrintInformation(RbxStu::Communication,
                                                 "Received HELLO. Re-Sending current configuration!");
                        auto hello = HelloPacket{};

                        memcpy(hello.szFingerprintHeader, communication->m_szFingerprintHeader.c_str(),
                               sizeof(hello.szFingerprintHeader));

                        hello.bIsSafeModeEnabled = !communication->m_bIsUnsafe;
                        hello.lCurrentExecutionDataModel = communication->lCurrentExecutionDataModel;
                        hello.bIsNativeCodeGenEnabled = communication->m_bEnableCodeGen;

                        auto generated = serializer->SerializeFromStructure(hello);
                        webSocket->send(std::string(generated.begin(), generated.end()), true);
                        sendResponseStatus = false;
                        break;
                    }
                    case RbxStu::WebSocketCommunication::SetFastVariablePacket:
                        break;
                    case RbxStu::WebSocketCommunication::SetFunctionBlockStatePacket: {
                        if (const auto packet =
                                    serializer->DeserializeFromString<SetRequestFingerprintPacket>(message->str);
                            packet.has_value()) {
                            communication->m_szFingerprintHeader = std::string(packet->vData->szNewFingerprint);
                            wasSuccess = true;
                        } else {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format(
                                            "WARNING: Packet dropped due to serialization problems! PacketId: {:#x}",
                                            static_cast<std::int32_t>(packetId)));
                        }
                        break;
                    }
                    case RbxStu::WebSocketCommunication::SetNativeCodeGenPacket: {
                        if (const auto packet = serializer->DeserializeFromString<SetNativeCodeGenPacket>(message->str);
                            packet.has_value()) {
                            communication->m_bEnableCodeGen = packet->vData->bEnableNativeCodeGen;
                            wasSuccess = true;
                        } else {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format(
                                            "WARNING: Packet dropped due to serialization problems! PacketId: {:#x}",
                                            static_cast<std::int32_t>(packetId)));
                        }

                        break;
                    }
                    case RbxStu::WebSocketCommunication::SetRequestFingerprintPacket: {
                        if (const auto packet =
                                    serializer->DeserializeFromString<SetRequestFingerprintPacket>(message->str);
                            packet.has_value()) {
                            communication->m_szFingerprintHeader = std::string(packet->vData->szNewFingerprint);
                            wasSuccess = true;
                        } else {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format(
                                            "WARNING: Packet dropped due to serialization problems! PacketId: {:#x}",
                                            static_cast<std::int32_t>(packetId)));
                        }
                        break;
                    }
                    case RbxStu::WebSocketCommunication::SetSafeModePacket: {
                        if (const auto packet = serializer->DeserializeFromString<SetSafeModePacket>(message->str);
                            packet.has_value()) {
                            communication->m_bIsUnsafe = !packet->vData->bIsSafeMode;
                            wasSuccess = true;
                        } else {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format(
                                            "WARNING: Packet dropped due to serialization problems! PacketId: {:#x}",
                                            static_cast<std::int32_t>(packetId)));
                        }

                        break;
                    }
                    case RbxStu::WebSocketCommunication::SetExecutionDataModelPacket:
                        break;
                    default:;
                }
                responseStatus.ullPacketFlags =
                        wasSuccess ? ResponseStatusPacketFlags::Success : ResponseStatusPacketFlags::Failure;
                auto generated = serializer->SerializeFromStructure(responseStatus);

                webSocket->send(std::string(generated.begin(), generated.end()), true);
                break;
            }
            case ix::WebSocketMessageType::Open: {
                logger->PrintInformation(RbxStu::Communication,
                                         "Sending HELLO to UI with all of RbxStu's current configuration!");
                auto hello = HelloPacket{};

                memcpy(hello.szFingerprintHeader, communication->m_szFingerprintHeader.c_str(),
                       sizeof(hello.szFingerprintHeader));

                hello.bIsSafeModeEnabled = !communication->m_bIsUnsafe;
                hello.lCurrentExecutionDataModel = communication->lCurrentExecutionDataModel;
                hello.bIsNativeCodeGenEnabled = communication->m_bEnableCodeGen;

                auto generated = serializer->SerializeFromStructure(hello);
                webSocket->send(std::string(generated.begin(), generated.end()), true);
                break;
            }
            case ix::WebSocketMessageType::Close:
                logger->PrintWarning(RbxStu::Communication, "WebSocket has been closed!");
                break;
            case ix::WebSocketMessageType::Error:
                break;
            case ix::WebSocketMessageType::Ping:
                break;

            case ix::WebSocketMessageType::Pong:
                break;

            case ix::WebSocketMessageType::Fragment:
                logger->PrintWarning(RbxStu::Communication, "Fragmented packets are not supported by the Communication "
                                                            "handler; The websocket will now close itself. Goodbye! "
                                                            "Reconnection will occur within 10 seconds!");
                webSocket->stop();
                break;
        }
    });

    while (true) {
        auto status = webSocket->connect(10);

        if (!status.success) {
            logger->PrintWarning(RbxStu::Communication,
                                 std::format("WebSocket connection failed! Status Code: '{}' (HTTP {}). ix::WebSocket "
                                             "error: '{}'. Reattempting in 5 seconds.",
                                             HttpStatus::ReasonPhrase(status.http_status), status.http_status,
                                             status.errorStr));
            std::this_thread::sleep_for(std::chrono::milliseconds{5000});
            continue;
        } else {
            logger->PrintInformation(
                    RbxStu::Communication,
                    std::format("WebSocket connection successful! Status Code: '{}' (HTTP {})! Launching handler task!",
                                HttpStatus::ReasonPhrase(status.http_status), status.http_status));
            webSocket->start();
        }

        while (webSocket->getReadyState() == ix::ReadyState::Open) {
            _mm_pause();
        }
        logger->PrintWarning(RbxStu::Communication, "Websocket connection lost. Beginning reconnection!");
        std::this_thread::sleep_for(std::chrono::milliseconds{5000});
    }
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
