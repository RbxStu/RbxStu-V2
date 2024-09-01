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
        char _8[1];
        char _9[3];
        char _c[1];
        char _d[3];
        char _10[4];
        char _14[4];
        char _18[2];
        char _1a[1];
        char _1b[4];
        char _1f[1];
        char _20[4];
        char _24[4];
        std::string m_szstdstringJobId;
        char _48[2];
        char _4a[6];
        char _50[6];
        char _56[2];
        std::uint64_t m_qwCreatorId;
        std::uint64_t m_qwGameId;
        std::uint64_t m_qwPlaceId;
        char _70[0x10];
        std::uint32_t m_dwPlaceVersion;
        char _88[0x20];
        char _a8[0x10];
        char _b8[8];
        char _c0[8];
        char _c8[0x38];
        char _100[8];
        char _108[0x38];
        char _140[0x30];
        char _170[0x10];
        char _180[0x10];
        void *m_pScriptContext;
        char _198[8];
        char _1a0[0x20];
        char _1c0[0x20];
        char _1e0[0x1c];
        char _1fc[4];
        char _200[8];
        char _208[0x2c];
        char _234[4];
        char _238[4];
        char _23c[4];
        char _240[0x10];
        char _250[0x20];
        char _270[0x10];
        char _280[0x10];
        char _290[8];
        std::uint64_t m_qwPrivateServerId;
        char _2a0[0x10];
        char _2b0[0xd];
        char _2bd[3];
        char _2c0[0x18];
        char _2d8[0x20];
        std::int64_t m_dwPlaceId;
        std::uint64_t m_qwPrivateServerOwnerId;
        char _308[0x38];
        char _340[0x30];
        char _370[8];
        char _378[8];
        char _380[4];
        char _384[2];
        char _386[0xa];
        char _390[8];
        char _398[8];
        char _3a0[8];
        char _3a8[0x18];
        char _3c0[0x30];
        char _3f0[8];
        char _3f8[8];
        bool m_bIsParallelPhase;
        char _404[4];
        char _408[8];
        char _410[0x30];
        char _440[0x20];
        enum RBX::DataModelType m_dwDataModelType;
        char _464[4];
        char _468[0x18];
        char _480[0x3c];
        char _4bc[4];
        char _4c0[0x40];
        char _500[0x40];
        char _540[1];
        char _541[1];
        char _542[1];
        bool m_bIsClosed;
        bool m_bIsContentLoaded;
        char _543[0x3b];
        char _580[0x40];
        char _5c0[0x20];
        char _5e0[0x10];
        bool m_bIsLoaded;
        char _5f1[2];
        bool m_bIsUniverseMetadataLoaded;
        char _5f4[0xc];
        char _600[0x10];
        char _610[0x30];
        char _640[0x28];
        char _668[8];
        char _670[8];
        char _678[8];
        char _680[8];
        char _688[8];
        char _690[8];
        char _698[0x28];
        char _6c0[0x40];
        char _700[0x31];
        char _731[0xf];
        char _740[8];
        char _748[4];
        char _74c[4];
        char _750[4];
        char _754[4];
        char _758[0x20];
        char _778[8];
        char _780[0x38];
        char _7b8[2];
        char _7ba[1];
        char _7bb[1];
        char _7bc[1];
        char _7bd[3];
        char _7c0[0x40];
        char _800[0x40];
        char _840[0x40];
        char _880[0x40];
        char _8c0[0x40];
        char _900[8];
        char _908[0x18];
        char _920[8];
        char _928[8];
        char _930[0xc];
        char _93c[1];
        bool m_bIsBeingClosed;
        std::int64_t m_qwGameLaunchIntent;
        char _948[0x18];
        char _960[4];
        char _964[1];
        char _965[0xb];
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


    enum PointerEncryptionType { ADD, SUB, XOR, UNDETERMINED };

    template<typename T>
    class PointerEncryption final {
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
} // namespace RBX
