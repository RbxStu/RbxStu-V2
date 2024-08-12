//
// Created by Dottik on 12/8/2024.
//

#pragma once
#include <cstdint>

namespace RBX::Console {
    enum MessageType : std::int32_t {
        Standard = 0,
        InformationBlue = 1,
        Warning = 2,
        Error = 3,
    };
} // namespace RBX::Console

namespace RBX {
    namespace Luau {
        enum TaskState : std::int8_t {
            None = 0,
            Deferred = 1,
            Delayed = 2,
            Waiting = 3,
        };
    }
} // namespace RBX