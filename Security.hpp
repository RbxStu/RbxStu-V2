//
// Created by Dottik on 16/8/2024.
//

#pragma once
#include <lapi.h>
#include <list>
#include <memory>

typedef int64_t (*Validator)(int64_t testAgainst, struct lua_State *testWith);

namespace RBX::Lua {
    struct ExtraSpace {
        struct Shared {
            int32_t threadCount;
            void *scriptContext;
            void *scriptVmState;
            char field_18[0x8];
            void *__intrusive_set_AllThreads;
        };

        char _1[8];
        char _8[8];
        char _10[8];
        struct RBX::Lua::ExtraSpace::Shared *sharedExtraSpace;
        char _20[8];
        Validator *CapabilitiesValidator;
        uint32_t identity;
        char _38[9];
        char _40[8];
        uint64_t capabilities;
        char _50[9];
        char _58[8];
        char _60[8];
        char _68[8];
        char _70[8];
        char _78[8];
        char _80[8];
        char _88[8];
        char _90[1];
        char _91[1];
        char _92[1];
        uint8_t taskStatus;
    };
    struct OriginalExtraSpace { // Used for pointer validation, DO NOT USE.
        char _1[8];
        char _8[8];
        char _10[8];
        struct RBX::Lua::ExtraSpace::Shared *sharedExtraSpace;
        char _20[8];
        Validator *CapabilitiesValidator;
        uint32_t identity;
        char _38[9];
        char _40[8];
        uint64_t capabilities;
        char _50[9];
        char _58[8];
        char _60[8];
        char _68[8];
        char _70[8];
        char _78[8];
        char _80[8];
        char _88[8];
        char _90[1];
        char _91[1];
        char _92[1];
        uint8_t taskStatus;
    };
} // namespace RBX::Lua

class Security final {
    /// @brief Private, Static shared pointer into the instance.
    static std::shared_ptr<Security> pInstance;

public:
    /// @brief Obtains the shared pointer that points to the global singleton for the current class.
    /// @return Singleton for Security as a std::shared_ptr<Security>.
    static std::shared_ptr<Security> GetSingleton();

    /// @brief Prints the capabilities to the console
    /// @param capabilities The capabilities
    void PrintCapabilities(std::uint32_t capabilities);

    /// @brief Converts identity to a capability
    /// @param identity The identity
    /// @remarks If identity is not specified in the identityCapabilities it will use the basic capability
    std::uint64_t IdentityToCapabilities(std::uint32_t identity);

    /// @brief Elevates a thread's identity and capability.
    /// @param L The lua state to elevate.
    /// @param identity
    /// @remarks This function will invoke the userthread callback with the parent thread as the L->global->mainthread
    /// if the given L does not contain a valid RobloxExtraSpace
    void SetThreadSecurity(lua_State *L, std::int32_t identity);

    /// @brief Validates if a given lua_State was created by RbxStu
    /// @param L The lua state to check.
    /// @remarks This function will only work if SetThreadSecurity(lua_State* L) was called, or if the threads'
    /// capabilities were set appropiately.
    /// @return true if the thread was created by RbxStu, false if it was not.
    bool IsOurThread(lua_State *L);

    /// @brief Elevates a lua closure's identity and capability.
    /// @param lClosure The closure to elevate.
    /// @param identity
    /// @remarks This function ONLY accepts lua closures. If the lua closure proto has no userdata, one will be created
    /// to store the required information.
    bool SetLuaClosureSecurity(Closure *lClosure, std::uint32_t identity);

    /// @brief Removes key information from a lua closure prototype.
    /// @param L The lua state the lClosure belongs to.
    /// @param lClosure The closure to wipe information from
    /// @remarks This function ONLY accepts lua closures, and it will modify the Closure *, beware of this.
    void WipeClosurePrototype(lua_State *L, const Closure *lClosure);
};

static_assert(sizeof(RBX::Lua::ExtraSpace) == 0x98,
              "RBX::Lua::ExtraSpace aka RobloxExtraSpace has a (known) size of 0x98 bytes.");

static_assert(sizeof(RBX::Lua::OriginalExtraSpace) == 0x98,
              "RBX::Lua::OriginalExtraSpace aka RobloxExtraSpace has a (known) size of 0x98 bytes.");
