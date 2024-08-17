//
// Created by Yoru on 8/16/2024.
//

#include <memory>
#include <string>

class Communication final {
public:
    /// @brief Swiftly handles the pipe used for executing Luau code.
    /// @param szPipeName The name of the pipe as a constant std::string.
    static void HandlePipe(const std::string &szPipeName);
};
