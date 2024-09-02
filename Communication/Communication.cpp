//
// Created by Yoru on 8/16/2024.
//

#include "Communication.hpp"

#include <HttpStatus.hpp>
#include <Windows.h>
#include "Environment/EnvironmentManager.hpp"
#include "Logger.hpp"
#include "PacketSerdes.hpp"
#include "Packets/HelloPacket.hpp"
#include "Packets/ResponseStatusPacket.hpp"
#include "Packets/ScheduleLuauPacket.hpp"
#include "Packets/SetExecutionDataModelPacket.hpp"
#include "Packets/SetFunctionBlockStatePacket.hpp"
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

void Communication::SetUnsafeMode(const bool isUnsafe) {
    const auto logger = Logger::GetSingleton();
    if (isUnsafe) {
        const auto ret = MessageBox(nullptr, "WARNING",
                                    "An attempt has been made to disable RbxStu's Safe Mode! This may constitute a "
                                    "malicious author "
                                    "trying to lower the security of the execution environment to steal ROBUX or "
                                    "other "
                                    "valuables! If you did not allow this, press No, and Safe Mode will remain "
                                    "active.",
                                    MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);

        if (ret == IDYES) {
            logger->PrintWarning(RbxStu::Communication, "RbxStu's Safe Mode has been disabled!");
            this->m_bIsUnsafe = isUnsafe;
        } else if (ret == IDNO) {
            logger->PrintWarning(RbxStu::Communication, "RbxStu's Safe Mode has not been disabled!");
        }
    } else {
        logger->PrintWarning(RbxStu::Communication, "RbxStu's Safe Mode has been reactivated!");
        this->m_bIsUnsafe = isUnsafe;
    }
}
std::string Communication::SetFingerprintHeader(const std::string &header) {
    if (header.find("-Fingerprint") == std::string::npos)
        return this->m_szFingerprintHeader;

    auto old = this->m_szFingerprintHeader;
    this->m_szFingerprintHeader = header;
    return old;
}
const std::string &Communication::GetFingerprintHeaderName() { return this->m_szFingerprintHeader; }
void Communication::SetExecutionDataModel(RBX::DataModelType dataModelType) {
    this->lCurrentExecutionDataModel = dataModelType;
}
const RBX::DataModelType Communication::GetExecutionDataModel() { return this->lCurrentExecutionDataModel; }
bool Communication::IsCodeGenerationEnabled() const { return this->m_bEnableCodeGen; }
void Communication::SetCodeGenerationEnabled(bool enableCodeGen) { this->m_bEnableCodeGen = enableCodeGen; }

[[noreturn]] void Communication::NewCommunication(const std::string &szRemoteHost) {
    const auto logger = Logger::GetSingleton();
    const auto scheduler = Scheduler::GetSingleton();
    const auto serializer = PacketSerdes::GetSingleton();
    const auto communication = Communication::GetSingleton();
    const auto environmentManager = EnvironmentManager::GetSingleton();

    if (szRemoteHost.find("ws://") == std::string::npos && szRemoteHost.find("wss://") == std::string::npos) {
        logger->PrintError(RbxStu::Communication, "Cannot begin ix::WebSocket! ");
        throw std::exception("szRemoteHost does not follow the protocol of ws:// or wss://! Cowardly refusing to begin "
                             "communication.");
    }

    logger->PrintInformation(RbxStu::Communication, "Starting ix::WebSocket for communication!");
    const auto webSocket = std::make_unique<ix::WebSocket>();
    webSocket->setUrl(szRemoteHost);
    webSocket->setOnMessageCallback([&environmentManager, &scheduler, &communication, &serializer, &logger,
                                     &webSocket](const ix::WebSocketMessagePtr &message) {
        switch (message->type) {
            case ix::WebSocketMessageType::Message: {
                if (message->binary) {
                    logger->PrintWarning(RbxStu::Communication,
                                         "RbxStu only supports non-binary inputs. Packet dropped.");
                    return;
                }

                auto responseStatus = ResponseStatusPacket{};
                responseStatus.ullPacketFlags = ResponseStatusPacketFlags::ResponseStatusSuccess;

                auto sendResponseStatus = true;
                auto wasSuccess = false;

                const auto packetId = static_cast<RbxStu::WebSocketCommunication::PacketIdentifier>(
                        serializer->ReadPacketIdentifier(message->str));

                switch (packetId) {
                    case RbxStu::WebSocketCommunication::HelloPacket: {
                        logger->PrintInformation(RbxStu::Communication,
                                                 "Received HELLO. Re-Sending current configuration!");
                        auto hello = HelloPacket{};
                        hello.szFingerprintHeader = communication->m_szFingerprintHeader;
                        hello.bIsSafeModeEnabled = !communication->m_bIsUnsafe;
                        hello.lCurrentExecutionDataModel = communication->lCurrentExecutionDataModel;
                        hello.bIsNativeCodeGenEnabled = communication->m_bEnableCodeGen;

                        const auto generated = serializer->SerializeFromStructure<HelloPacket>(hello);
                        webSocket->send(generated.dump(), false);
                        sendResponseStatus = false;
                        break;
                    }
                    case RbxStu::WebSocketCommunication::SetFastVariablePacket:
                        break;
                    case RbxStu::WebSocketCommunication::SetFunctionBlockStatePacket: {
                        if (const auto packet =
                                    serializer->DeserializeFromJson<SetFunctionBlockStatePacket>(message->str);
                            packet.has_value()) {
                            environmentManager->SetFunctionBlocked(packet->szFunctionName,
                                                                   packet->ullPacketFlags !=
                                                                           SetFunctionBlockStatePacketFlags::Allow);
                            wasSuccess = true;
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format("The unsafe function list has been modified! Change "
                                                "Set:\nFunction Name: '{}'\nIs Blocked: '{}'",
                                                packet->szFunctionName,
                                                packet->ullPacketFlags != SetFunctionBlockStatePacketFlags::Allow
                                                        ? "Yes"
                                                        : "No"));
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
                        if (const auto packet = serializer->DeserializeFromJson<SetNativeCodeGenPacket>(message->str);
                            packet.has_value()) {
                            communication->m_bEnableCodeGen = packet->bEnableNativeCodeGen;
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
                                    serializer->DeserializeFromJson<SetRequestFingerprintPacket>(message->str);
                            packet.has_value()) {
                            communication->m_szFingerprintHeader = std::string(packet->szNewFingerprint);
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
                        if (const auto packet = serializer->DeserializeFromJson<SetSafeModePacket>(message->str);
                            packet.has_value()) {
                            communication->SetUnsafeMode(!packet.value().bIsSafeMode);
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
                        if (const auto packet =
                                    serializer->DeserializeFromJson<SetExecutionDataModelPacket>(message->str);
                            packet.has_value()) {

                            switch (static_cast<SetExecutionDataModelPacketFlags>(packet.value().ullPacketFlags)) {
                                case Edit:
                                    scheduler->SetExecutionDataModel(RBX::DataModelType::DataModelType_Edit);
                                    break;
                                default:
                                case Client:
                                    scheduler->SetExecutionDataModel(RBX::DataModelType::DataModelType_PlayClient);
                                    break;
                                case Server:
                                    scheduler->SetExecutionDataModel(RBX::DataModelType::DataModelType_PlayServer);
                                    break;
                                case Standalone:
                                    scheduler->SetExecutionDataModel(
                                            RBX::DataModelType::DataModelType_MainMenuStandalone);
                                    break;
                            }

                            wasSuccess = true;
                        } else {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format(
                                            "WARNING: Packet dropped due to serialization problems! PacketId: {:#x}",
                                            static_cast<std::int32_t>(packetId)));
                        }
                        break;

                    case RbxStu::WebSocketCommunication::ScheduleLuauPacket:
                        if (const auto packet = serializer->DeserializeFromJson<ScheduleLuauPacket>(message->str);
                            packet.has_value()) {
                            if (scheduler->IsInitialized()) {
                                logger->PrintInformation(RbxStu::Communication, "Luau code scheduled for execution.");
                                scheduler->ScheduleJob(SchedulerJob(packet.value().szLuauCode));
                            } else {
                                logger->PrintWarning(
                                        RbxStu::Communication,
                                        "RbxStu::Scheduler is NOT initialized! Luau code NOT scheduled, please enter "
                                        "into a "
                                        "game to be able to enqueue jobs into the Scheduler for execution!");
                            }

                            wasSuccess = true;
                        } else {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format(
                                            "WARNING: Packet dropped due to serialization problems! PacketId: {:#x}",
                                            static_cast<std::int32_t>(packetId)));
                        }

                        break;
                    default:;
                }

                if (!sendResponseStatus)
                    break;

                responseStatus.ullPacketFlags = wasSuccess ? ResponseStatusPacketFlags::ResponseStatusSuccess
                                                           : ResponseStatusPacketFlags::ResponseStatusFailure;

                const auto generated = serializer->SerializeFromStructure(responseStatus);
                webSocket->send(generated.dump(), false);
                break;
            }
            case ix::WebSocketMessageType::Open: {
                logger->PrintInformation(RbxStu::Communication,
                                         "Sending HELLO to UI with all of RbxStu's current configuration!");
                auto hello = HelloPacket{};
                hello.szFingerprintHeader = communication->m_szFingerprintHeader;
                hello.bIsSafeModeEnabled = !communication->m_bIsUnsafe;
                hello.lCurrentExecutionDataModel = communication->lCurrentExecutionDataModel;
                hello.bIsNativeCodeGenEnabled = communication->m_bEnableCodeGen;

                const auto generated = serializer->SerializeFromStructure(hello);
                webSocket->send(generated.dump(), false);
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

    auto attemptCount = 0;
    while (attemptCount <= 3) {

        if (auto status = webSocket->connect(10); !status.success) {
            logger->PrintWarning(RbxStu::Communication,
                                 std::format("WebSocket connection failed! Status Code: '{}' (HTTP {}). ix::WebSocket "
                                             "error: '{}'. Reattempting in 5 seconds.",
                                             HttpStatus::ReasonPhrase(status.http_status), status.http_status,
                                             status.errorStr));
            std::this_thread::sleep_for(std::chrono::milliseconds{5000});
            attemptCount++;
            continue;
        } else {
            logger->PrintInformation(
                    RbxStu::Communication,
                    std::format("WebSocket connection successful! Status Code: '{}' (HTTP {})! Launching handler task!",
                                HttpStatus::ReasonPhrase(status.http_status), status.http_status));
            webSocket->start();
            attemptCount = 0;
        }

        while (webSocket->getReadyState() == ix::ReadyState::Open) {
            if (!communication->m_qExecutionReportsQueue.empty()) {
                auto [operationStatus, szOperationIdentifier, szErrorMessage] =
                        communication->m_qExecutionReportsQueue.front();
                communication->m_qExecutionReportsQueue.pop();
                ScheduleLuauResponsePacket response{};
                response.ullPacketFlags = operationStatus;
                response.szText = szErrorMessage;
                response.szOperationIdentifier = szOperationIdentifier;
                webSocket->send(serializer->SerializeFromStructure(response), false);
                std::this_thread::sleep_for(std::chrono::milliseconds{5});
            } else {
                _mm_pause();
            }
        }
        logger->PrintWarning(RbxStu::Communication, "Websocket connection lost. Beginning reconnection!");
        std::this_thread::sleep_for(std::chrono::milliseconds{5000});
    }

    logger->PrintError(RbxStu::Communication,
                       "WebSocket communication attempts ceased, only Pipe connection will remain online.");
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
void Communication::ReportExecutionStatus(const ExecutionStatus &execStatus) {
    this->m_qExecutionReportsQueue.emplace(execStatus);
}
