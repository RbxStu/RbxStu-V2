//
// Created by Dottik on 13/8/2024.
//

#pragma once
#include <map>
#include <memory>
#include <string>

/// @brief Manages the way RbxStu interacts with Luau.
class LuauManager final {
    /// @brief Private, Static shared pointer into the instance.
    static std::shared_ptr<LuauManager> pInstance;

    /// @brief The map for the original Luau functions for when a Luau functions is hooked. Guaranteed to not contain
    /// any hooked functions
    std::map<std::string, void *> m_mapHookMap;

    /// @brief The map for Luau functions. May include hooked functions.
    std::map<std::string, void *> m_mapLuauFunctions;

    /// @brief Whether the current instance is initialized.
    bool m_bIsInitialized = false;

    /// @brief Initializes the LuauManager instance, obtaining all functions from their respective signatures and
    /// establishing the initial hooks required for the manager to operate as expected.
    void Initialize();

public:
    /// @brief Obtains the shared pointer that points to the global singleton for the current class.
    /// @return Singleton for LuauManager as a std::shared_ptr<LuauManager>.
    static std::shared_ptr<LuauManager> GetSingleton();

    bool IsInitialized() const;

    /// @brief Obtains the original function given the function's name.
    /// @param functionName The name of the Luau function to obtain the original from.
    /// @note This function may return non-hooked functions as well.
    /// @remark WARNING ON USAGE: This function is for internal usage of LuauManager, whilst callers may use it to
    /// obtain the original version of a Luau function on the remote Roblox environment or to hook it themselves, this
    /// is discouraged, and wrong, do NOT do that.
    /// @return A type-less pointer into the start of the original function.
    void *GetHookOriginal(const std::string &functionName);

    void *GetFunction(const std::string &functionName);
};
