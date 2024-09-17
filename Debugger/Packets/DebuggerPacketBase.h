//
// Created by Dottik on 15/9/2024.
//

#include "../../Communication/Packets/PacketBase.hpp"

namespace RbxStu {
    enum DebuggerCommunication {
        Notify_BreakPointReached = 0x0,
        SetBreakpoint = 0x1,
    };
};

class DebuggerPacketBase : public PacketFunctions {
public:
};
