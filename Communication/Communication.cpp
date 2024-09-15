//
// Created by Yoru on 8/16/2024.
//

#include "Communication.hpp"

#include <HttpStatus.hpp>
#include <Windows.h>
#include "Environment/EnvironmentManager.hpp"
#include "Logger.hpp"
#include "PacketSerdes.hpp"
#include "Packets/DataModelUpdatePacket.hpp"
#include "Packets/HelloPacket.hpp"
#include "Packets/HttpConfigurationPacket.hpp"
#include "Packets/ResponseStatusPacket.hpp"
#include "Packets/ScheduleLuauPacket.hpp"
#include "Packets/SetExecutionDataModelPacket.hpp"
#include "Packets/SetFastVariablePacket.hpp"
#include "Packets/SetFunctionBlockStatePacket.hpp"
#include "Packets/SetNativeCodeGenPacket.hpp"
#include "Packets/SetSafeModePacket.hpp"
#include "Packets/SetScriptSourceAccessPacket.hpp"
#include "RobloxManager.hpp"
#include "Scheduler.hpp"

#include "ixwebsocket/IXWebSocket.h"

std::shared_ptr<Communication> Communication::pInstance;

static std::mutex __get_singleton_lock{};

Communication::Communication() {
    if (const auto hwid = Utilities::GetHwid(); hwid.has_value())
        this->m_szHardwareId = hwid.value();
    else
        this->m_szHardwareId = "8F3A2C1BE9D70F4";
}

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
        const auto ret = MessageBox(nullptr,
                                    "An attempt has been made to disable RbxStu's Safe Mode! This may constitute a "
                                    "malicious author "
                                    "trying to lower the security of the execution environment to steal ROBUX or "
                                    "other "
                                    "valuables! If you did not allow this, press No, and Safe Mode will remain "
                                    "active.",
                                    "WARNING", MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);

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

void Communication::OnDataModelUpdated(const RBX::DataModelType dataModelType, const bool wasCreated) {
    if (WebSocket.get() && WebSocket->getReadyState() == ix::ReadyState::Open) {
        const auto serializer = PacketSerdes::GetSingleton();
        const auto dataModelUpdated = DataModelUpdatePacket{dataModelType, wasCreated};

        const auto generated = serializer->SerializeFromStructure<DataModelUpdatePacket>(dataModelUpdated);
        this->WebSocket->send(generated.dump(), false);
    }
}
void Communication::SetExecutionDataModel(RBX::DataModelType dataModelType) {
    this->lCurrentExecutionDataModel = dataModelType;
    Scheduler::GetSingleton()->ResetScheduler(SchedulerResetReason::ExecutionDataModelChanged);
}
const RBX::DataModelType Communication::GetExecutionDataModel() { return this->lCurrentExecutionDataModel; }
bool Communication::IsCodeGenerationEnabled() const { return this->m_bEnableCodeGen; }
void Communication::SetCodeGenerationEnabled(const bool enableCodeGen) { this->m_bEnableCodeGen = enableCodeGen; }
bool Communication::CanAccessScriptSource() const { return this->m_bAllowScriptSourceAccess; }
std::string Communication::GetHardwareId() { return this->m_szHardwareId; }

[[noreturn]] void Communication::NewCommunication(const std::string &szRemoteHost) {
    const auto logger = Logger::GetSingleton();
    const auto scheduler = Scheduler::GetSingleton();
    const auto serializer = PacketSerdes::GetSingleton();
    const auto communication = Communication::GetSingleton();
    const auto environmentManager = EnvironmentManager::GetSingleton();
    const auto robloxManager = RobloxManager::GetSingleton();

    if (szRemoteHost.find("ws://") == std::string::npos && szRemoteHost.find("wss://") == std::string::npos) {
        logger->PrintError(RbxStu::Communication, "Cannot begin ix::WebSocket! ");
        throw std::exception("szRemoteHost does not follow the protocol of ws:// or wss://! Cowardly refusing to begin "
                             "communication.");
    }

    logger->PrintInformation(RbxStu::Communication, "Starting ix::WebSocket for communication!");
    auto webSocket = std::make_shared<ix::WebSocket>();
    communication->WebSocket = webSocket;
    webSocket->setUrl(szRemoteHost);
    webSocket->setOnMessageCallback([&robloxManager, &environmentManager, &scheduler, &communication, &serializer,
                                     &logger, &webSocket](const ix::WebSocketMessagePtr &message) {
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

                    case RbxStu::WebSocketCommunication::SetFastVariablePacket: {
                        if (const auto packet = serializer->DeserializeFromJson<SetFastVariablePacket>(message->str);
                            packet.has_value()) {
                            const auto variableName = packet->szFastVariableName;

                            if (const auto fastVar = robloxManager->GetFastVariable(variableName);
                                fastVar.has_value()) {
                                wasSuccess = true;
                                void *dataSegmentPointer = fastVar.value();
                                logger->PrintInformation(RbxStu::Communication,
                                                         std::format("Fast Variable modified: {}. Writing value to "
                                                                     "pointer -> '{}'. Packet Flag: {:#x}",
                                                                     packet->szFastVariableName, dataSegmentPointer,
                                                                     packet->ullPacketFlags));


                                switch (static_cast<SetFastVariablePacketFlag>(packet->ullPacketFlags)) {
                                    case SetFastVariablePacketFlag::Boolean: {
                                        *static_cast<bool *>(dataSegmentPointer) = packet->bNewValue;
                                        break;
                                    }
                                    case SetFastVariablePacketFlag::Integer: {
                                        *static_cast<std::int32_t *>(dataSegmentPointer) = packet->lNewValue;
                                        break;
                                    }
                                    case SetFastVariablePacketFlag::String: {
                                        // Swap pointers.
                                        auto oldString = *static_cast<char **>(dataSegmentPointer);
                                        auto s = new char[packet->szNewValue.size() + 1];
                                        memcpy(s, packet->szNewValue.c_str(), packet->szNewValue.size());
                                        s[packet->szNewValue.size()] = 0;
                                        *static_cast<char **>(dataSegmentPointer) = s;
                                        delete[] oldString;
                                        break;
                                    }
                                    default: {
                                        logger->PrintError(
                                                RbxStu::Communication,
                                                "Cannot mutate Fast Variable, as the value that it contains is "
                                                "unknown, and manipulating it could constitute data-loss or memory "
                                                "corruption! Validate your packet flags.");
                                        wasSuccess = false;
                                        break;
                                    }
                                }
                            } else {
                                logger->PrintWarning(RbxStu::Communication,
                                                     std::format("Failed to change Fast Variable with name '{}' as "
                                                                 "RobloxManager was unable to track it down.",
                                                                 variableName));
                            }
                        } else {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    std::format(
                                            "WARNING: Packet dropped due to serialization problems! PacketId: {:#x}",
                                            static_cast<std::int32_t>(packetId)));
                        }
                        break;
                    }

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

                    case RbxStu::WebSocketCommunication::HttpConfigurationPacket: {
                        if (const auto packet = serializer->DeserializeFromJson<HttpConfigurationPacket>(message->str);
                            packet.has_value()) {

                            if (packet->szNewFingerprint.empty()) {
                                logger->PrintWarning(RbxStu::Communication,
                                                     "HttpConfigurationPacket provided no new Fingerprint. "
                                                     "Automatically using the DEFAULT RbxStu-Fingerprint!");
                                communication->m_szFingerprintHeader = "RbxStu-Fingerprint";
                            } else {
                                communication->m_szFingerprintHeader = std::string(packet->szNewFingerprint);
                            }

                            if (packet->szNewHwid.empty()) {
                                logger->PrintWarning(RbxStu::Communication,
                                                     "HttpConfigurationPacket provided no new HardwareID. "
                                                     "Automatically using the DEFAULT system hardware ID.");
                                if (const auto hwid = Utilities::GetHwid(); hwid.has_value())
                                    communication->m_szHardwareId = hwid.value();
                                else
                                    logger->PrintError(RbxStu::Communication,
                                                       "Hardware ID not changed! There has been an error obtaining "
                                                       "your PC's Hardware ID!");
                            } else {
                                communication->m_szHardwareId = std::string(packet->szNewHwid);
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

                    case RbxStu::WebSocketCommunication::SetExecutionDataModelPacket: {
                        if (const auto packet =
                                    serializer->DeserializeFromJson<SetExecutionDataModelPacket>(message->str);
                            packet.has_value()) {
                            logger->PrintWarning(
                                    RbxStu::Communication,
                                    "WARNING: Changing the DataModel while in execution MAY result in crashes! Avoid "
                                    "changing DataModels after making something that may affect RbxStu, like using "
                                    "hookfunction! Changing DataModels is supported, but its not the most polished "
                                    "functionality, so it may lead to crashes or undefined behavior, avoid it if possible!");


                            switch (static_cast<SetExecutionDataModelPacketFlags>(packet.value().ullPacketFlags)) {
                                case SetExecutionDataModelPacketFlags::Edit:
                                    logger->PrintWarning(RbxStu::Communication,
                                                         "WARNING: The execution DataModel has been changed to Edit "
                                                         "mode. This may allow for things like complete saveinstance "
                                                         "into a remote server. This is dangerous!");
                                    communication->SetExecutionDataModel(RBX::DataModelType::DataModelType_Edit);
                                    break;
                                default:
                                case SetExecutionDataModelPacketFlags::Client:
                                    communication->SetExecutionDataModel(RBX::DataModelType::DataModelType_PlayClient);
                                    break;
                                case SetExecutionDataModelPacketFlags::Server:
                                    logger->PrintWarning(
                                            RbxStu::Communication,
                                            "WARNING: The execution DataModel has been changed to Server "
                                            "mode. This may result in unknown behaviour or security risks.");
                                    communication->SetExecutionDataModel(RBX::DataModelType::DataModelType_PlayServer);
                                    break;
                                case SetExecutionDataModelPacketFlags::Standalone:
                                    logger->PrintWarning(
                                            RbxStu::Communication,
                                            "WARNING: The execution DataModel has been changed to Standalone "
                                            "mode. This may result in unknown behaviour or security risks.");

                                    communication->SetExecutionDataModel(
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
                    }

                    case RbxStu::WebSocketCommunication::ScheduleLuauPacket: {
                        if (const auto packet = serializer->DeserializeFromJson<ScheduleLuauPacket>(message->str);
                            packet.has_value()) {
                            if (scheduler->IsInitialized()) {
                                logger->PrintInformation(RbxStu::Communication,
                                                         std::format("Luau code scheduled for execution; JobID: '{}'",
                                                                     packet.value().szOperationIdentifier));
                                scheduler->ScheduleJob(
                                        SchedulerJob(packet.value().szLuauCode, packet.value().szOperationIdentifier));
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
                    }

                    case RbxStu::WebSocketCommunication::SetScriptSourceAccessPacket: {
                        if (const auto packet =
                                    serializer->DeserializeFromJson<SetScriptSourceAccessPacket>(message->str);
                            packet.has_value()) {
                            const auto sourceAccess = packet.value().ullPacketFlags ==
                                                      SetScriptSourceAccessPacketFlags::AllowSourceAccess;
                            if (sourceAccess) {
                                const auto ret = MessageBox(
                                        nullptr,
                                        "An attempt enable .Source access to executed scripts! This may lead to your "
                                        "entire game being at risk of being dumped by malicious actors who recognize "
                                        "the RbxStu V2 environment. This is the ONLY warning you will receive, this is "
                                        "considered a security risk, as it may lead to the opposite of the purpose of "
                                        "RbxStu, which is to contribute to stopping cheats/improve developer "
                                        "experience. Only allow .Source access when you trust the script "
                                        "provider/author!",
                                        "WARNING",

                                        MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);

                                if (ret == IDYES) {
                                    logger->PrintWarning(RbxStu::Communication,
                                                         "Access to .Source on scripts has been allowed.!");
                                    communication->m_bAllowScriptSourceAccess = sourceAccess;
                                    environmentManager->SetServiceBlocked("ScriptEditorService", false);
                                } else if (ret == IDNO) {
                                    logger->PrintWarning(RbxStu::Communication,
                                                         "Access to .Source on scripts has been kept disabled!");
                                    environmentManager->SetServiceBlocked("ScriptEditorService", true);
                                }
                            } else {
                                logger->PrintWarning(RbxStu::Communication, "Access to .Source has been disabled");
                                communication->m_bAllowScriptSourceAccess = sourceAccess;
                                environmentManager->SetServiceBlocked("ScriptEditorService", true);
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
                    }

                    default:
                        logger->PrintWarning(RbxStu::Communication, std::format("Unhandled PacketID! PacketID: {:#x}",
                                                                                static_cast<std::int32_t>(packetId)));
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
                if (webSocket->getReadyState() == ix::ReadyState::Open) {
                    logger->PrintInformation(RbxStu::Communication,
                                             "Sending HELLO to UI with all of RbxStu's current configuration!");
                    auto hello = HelloPacket{};
                    hello.szFingerprintHeader = communication->m_szFingerprintHeader;
                    hello.bIsSafeModeEnabled = !communication->m_bIsUnsafe;
                    hello.lCurrentExecutionDataModel = communication->lCurrentExecutionDataModel;
                    hello.bIsNativeCodeGenEnabled = communication->m_bEnableCodeGen;

                    const auto generated = serializer->SerializeFromStructure(hello);
                    webSocket->send(generated.dump(), false);
                }
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
                auto status = communication->m_qExecutionReportsQueue.front();
                ScheduleLuauResponsePacket response{};
                response.ullPacketFlags = status.Status;
                response.szText = status.szErrorMessage;
                response.szOperationIdentifier = status.szOperationIdentifier;
                webSocket->send(serializer->SerializeFromStructure(response).dump(), false);
                communication->m_qExecutionReportsQueue.pop();
                std::this_thread::sleep_for(std::chrono::milliseconds{5});
            } else {
                _mm_pause();
            }
        }
        logger->PrintWarning(RbxStu::Communication, "Websocket connection lost. Beginning reconnection!");
        webSocket->stop();
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
