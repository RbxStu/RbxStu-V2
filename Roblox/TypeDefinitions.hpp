//
// Created by Dottik on 12/8/2024.
//

#pragma once
#include <cstdint>

#include "lstate.h"
namespace RBX {
    struct SystemAddress {
        struct PeerId {
            uint32_t peerId;
        };

        PeerId remoteId;
    };

    namespace Lua {
        struct WeakThreadRef {
            std::atomic_int32_t _Refs;
            lua_State *thread;
            int32_t thread_ref;
            int32_t objectId;
        };
    } // namespace Lua
    namespace Console {
        enum MessageType : std::int32_t {
            Standard = 0,
            InformationBlue = 1,
            Warning = 2,
            Error = 3,
        };
    } // namespace Console

    namespace Luau {
        enum TaskState : std::int8_t {
            None = 0,
            Deferred = 1,
            Delayed = 2,
            Waiting = 3,
        };
    } // namespace Luau

    enum DataModelType : std::int32_t {
        DataModelType_Edit = 0,
        DataModelType_PlayClient = 1,
        DataModelType_PlayServer = 2,
        DataModelType_MainMenuStandalone = 3,
        DataModelType_Null = 4
    };

    static const char *GetStringFromSharedString(void *sharedString) {
        // Obtained from finding LuaVM::Load. The second or third argument is the scripts' source.
        // The string is an RBX::ProtectedString. RBX::ProtectedString holds two pointers, one to source and one to
        // bytecode. The bytecode is kept at protectedString + 0x8, while the source is kept at protectedString + 0x0.
        // Keep this in mind if this update changes!

        // The *sharedDeref + 0x10 pointer arithmetic comes from the function called to obtain the string from the
        // RBX::ProtectedString instnace.


        auto sharedDeref = *reinterpret_cast<void **>(sharedString);
        return reinterpret_cast<const char *>(reinterpret_cast<std::uintptr_t>(sharedDeref) + 0x10);
    }

    static std::string DataModelTypeToString(const std::int32_t num) {
        if (num == 0) {
            return "StudioGameStateType_Edit";
        }
        if (num == 1) {
            return "StudioGameStateType_PlayClient";
        }
        if (num == 2) {
            return "StudioGameStateType_PlayServer";
        }
        if (num == 3) {
            return "StudioGameStateType_Standalone";
        }
        if (num == 4) {
            return "StudioGameStateType_Null";
        }
        return "<Invalid StudioGameStateType>";
    }

    struct DataModel {
        char _0[0x50];
        char _50[0x50];
        char _a0[0x50];
        char _f0[0x50];
        char _140[0x50];
        char _190[8];
        char _198[8];
        char _1a0[0x40];
        char _1e0[0x50];
        char _230[0x40];
        char _270[0x10];
        char _280[0x10];
        char _290[0x40];
        char _2d0[8];
        char _2d8[0x48];
        char _320[0x50];
        char _370[0x10];
        char _380[0x18];
        char _398[0x28];
        char _3c0[0x50];
        char _410[0x50];
        /// @brief Describes the DataModel type.
        enum RBX::DataModelType m_dwDataModelType;
        char _464[0x4c];
        char _4b0[0x50];
        char _500[0x41];
        char _541[1];
        char _542[1];
        /// @brief Whether the current DataModel is marked as "Closed" (closed by RBX::DataModel::doCloseDataModel).
        bool m_bIsClosed;
    };

    struct ModuleScript {
        char _0[8];
        char _8[0x38];
        char _40[0x40];
        char _80[0x40];
        char _c0[0x40];
        char _100[0x40];
        char _140[0x40];
        char _180[0x18];
        bool m_bIsRobloxScriptModule;
    };


} // namespace RBX
