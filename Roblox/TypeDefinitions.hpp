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
        void *vftable;
        char _8[0x28];
        std::string m_szstdstringJobId;
        char _50[0x10];
        uint64_t m_qwCreatorId;
        uint64_t m_qwGameId;
        uint64_t m_qwPlaceId;
        char _78[8];
        char _80[8];
        uint32_t m_dwPlaceVersion;
        char _90[0x30];
        char _c0[0x40];
        char _100[0x40];
        char _140[0x40];
        char _180[0x10];
        char _190[8];
         void *m_pScriptContext;
        char _1a0[0x20];
        char _1c0[0x40];
        char _200[0x40];
        char _240[0x38];
        char _278[8];
        char _280[0x18];
        char _298[8];
        uint64_t m_qwPrivateServerId;
        char _2a8[0x18];
        char _2c0[0x20];
        char _2e0[0x20];
        int64_t m_dwPlaceId;
        uint64_t m_qwPrivateServerOwnerId;
        char _310[0x30];
        char _340[0x40];
        char _380[8];
        char _388[0x38];
        char _3c0[0x40];
        char _400[8];
        bool m_bIsParallelPhase;
        char _40c[0x34];
        char _440[0x28];
        enum RBX::DataModelType m_dwDataModelType;
        char _46c[0x14];
        char _480[0x40];
        char _4c0[0x40];
        char _500[0x40];
        char _540[9];
        char _549[1];
        char _54a[1];
        bool m_bIsClosed;
        bool m_bIsContentLoaded;
        char _54d[0x33];
        char _580[0x40];
        char _5c0[0x28];
        char _5e8[0x10];
        bool m_bIsLoaded;
        char _5f9[2];
        bool m_bIsUniverseMetadataLoaded;
        char _5fc[4];
        char _600[0x40];
        char _640[0x40];
        char _680[0x40];
        char _6c0[0x40];
        char _700[0x40];
        char _740[0x40];
        char _780[0x40];
        char _7c0[0x40];
        char _800[0x40];
        char _840[0x40];
        char _880[0x40];
        char _8c0[0x40];
        char _900[0x28];
        char _928[0x18];
        char _940[5];
        bool m_bIsBeingClosed;
        int64_t m_qwGameLaunchIntent;
        char _950[0x28];
    };


    struct ModuleScript {
        char _0[8];
        char _1[8];
        char _8[0x38];
        char _40[0x40];
        char _80[0x40];
        char _c0[0x40];
        char _100[0x40];
        char _140[0x40];
        char _180[0x18];
        bool m_bIsRobloxScriptModule;
    };


    enum PointerEncryptionType { ADD, SUB, XOR, UNDETERMINED };

    template<typename T>
    class PointerEncryption final {
    public:
        std::uintptr_t addressOne;
        std::uintptr_t *addressTwo;

        T DecodePointerWithOperation(const PointerEncryptionType pointerEncryption) {
            switch (pointerEncryption) {
                case ADD:
                    return addressOne + *addressTwo;
                case XOR:
                    return addressOne ^ *addressTwo;
                case SUB:
                    return addressOne - *addressTwo;
                default:
                    return *addressTwo - addressOne;
            }
        }
    };

    template<typename T>
    class PointerOffsetEncryption final {
    public:
        std::uintptr_t address = 0;
        std::uintptr_t offset = 0;

        PointerOffsetEncryption(void *address, std::uintptr_t offset) {
            this->address = reinterpret_cast<std::uintptr_t>(address);
            this->offset = offset;
        }

        T *DecodePointerWithOffsetEncryption(const PointerEncryptionType pointerEncryption) {
            switch (pointerEncryption) {
                case ADD:
                    return reinterpret_cast<T *>(address + offset);
                case SUB:
                    return reinterpret_cast<T *>(address - offset);
                default:
                    return nullptr;
            }
        }
    };
} // namespace RBX
