//
// Created by Yoru on 8/16/2024.
//
#pragma once

#include <memory>
#include <queue>
#include <string>

#include "Packets/ScheduleLuauResponsePacket.hpp"
#include "Roblox/TypeDefinitions.hpp"

struct ExecutionStatus {
    ScheduleLuauResponsePacketFlags Status;
    std::string szOperationIdentifier;
    std::string szErrorMessage;
};

class Communication final {
    static std::shared_ptr<Communication> pInstance;

    bool m_bIsUnsafe = false;
    bool m_bEnableCodeGen = false;
    bool m_bIsInitialized = false;
    bool m_bAllowScriptSourceAccess = false;

    std::queue<ExecutionStatus> m_qExecutionReportsQueue;

    RBX::DataModelType lCurrentExecutionDataModel = RBX::DataModelType_PlayClient;

    std::string m_szFingerprintHeader = "RbxStu-Fingerprint";


public:
    static std::shared_ptr<Communication> GetSingleton();

    /// @brief Defines if the DLL should run in UNSAFE mode, turning off all protections, leaving raw execution.
    /// @return True if execution is unsafe, false if it is not.
    /// @remarks If this returns false, this means you're running in unsafe mode, whilst it provides more flexibility,
    /// it gives access to functions you otherwise would not have access to, this is perfect for local experiments, but
    /// for running unknown scripts, this MUST be false, else you are at risk of being compromised!
    bool IsUnsafeMode() const;

    /// @brief Allows access to enabling and disabling unsafe mode, disabling and enabling protections respectively.
    /// @param isUnsafe If true, security will be disabled, if false, security will be enabled back.
    /// @remarks THIS IS A DANGEROUS FUNCTIOn, AND MAY EXPOSE THE USER TO TERRIBLE THINGS! CALL AT YOUR OWN RISK!
    void SetUnsafeMode(bool isUnsafe);


    /// @brief Allows to set the Fingerprint header used to identify RbxStu, used to impersonate other executors.
    /// @param header The new header.
    /// @return Returns the old Fingerprint header.
    /// @remarks Will not set the fingerprint header to "header" if "header" does not contain "-Fingerprint"
    std::string SetFingerprintHeader(const std::string &header);

    const std::string &GetFingerprintHeaderName();

    void SetExecutionDataModel(RBX::DataModelType dataModelType);

    const RBX::DataModelType GetExecutionDataModel();

    bool IsCodeGenerationEnabled() const;
    void SetCodeGenerationEnabled(bool enableCodeGen);
    bool CanAccessScriptSource() const;

    /// @brief Swiftly handles the WebSocket used for advanced communication with RbxStu V2.
    /// @param szRemoteHost The remote host that will handle the communication.
    static void NewCommunication(const std::string &szRemoteHost);

    /// @brief Swiftly handles the pipe used for executing Luau code.
    /// @param szPipeName The name of the pipe as a constant std::string.
    static void HandlePipe(const std::string &szPipeName);

    void ReportExecutionStatus(const ExecutionStatus &execStatus);
};
