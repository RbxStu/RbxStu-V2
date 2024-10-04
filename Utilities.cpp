//
// Created by Dottik on 10/8/2024.
//

#include "Utilities.hpp"

void Utilities::Initialize() {
    try {
        RbxStu::Regexes::LUA_ERROR_CALLSTACK_REGEX =
                std::regex(R"(.*"\]:(\d)*: )", std::regex::optimize | std::regex::icase);
    } catch (const std::exception &ex) {
        Logger::GetSingleton()->PrintError(RbxStu::Anonymous, std::format("wow: {}", ex.what()));
    }
}

std::string Utilities::StripLuaErrorMessage(const std::string &message) {
    if (std::regex_search(message.begin(), message.end(), RbxStu::Regexes::LUA_ERROR_CALLSTACK_REGEX)) {
        const auto fixed = std::regex_replace(message, RbxStu::Regexes::LUA_ERROR_CALLSTACK_REGEX, "");
        return fixed;
    }
    return message;
}
