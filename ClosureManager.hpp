//
// Created by Dottik on 21/8/2024.
//

#pragma once

#include <lobject.h>
#include <lua.h>
#include <map>

struct LuauHookInformation {
    /// @brief Used to quickly distinguish between a C function and a Lua function.
    bool isC;
    /// @brief Whether or not the hook was a C->L or L->C hook, which cannot be unhooked due to its nature.
    bool isWrappedHook;
    /// @brief A clone of the original closure, not returned by hookfunction.
    /// must be referenced on the lua registry, else, it may be as well useless, due to it being GCd.
    Closure *originalClosureClone;
    /// @brief The replaced closure on the environment.
    Closure *hookedClosure;
};

class ClosureManager final {
    static std::shared_ptr<ClosureManager> pInstance;

    std::map<Closure *, LuauHookInformation> m_hookMap;
    std::map<Closure *, int> m_newcclosureMap;

    /// @brief Handles a newcclosure call.
    static int newcclosure_handler(lua_State *L);

public:
    static std::shared_ptr<ClosureManager> GetSingleton();

    bool IsWrappedCClosure(Closure *cl) const;

    /// @brief Hooks two functions present at the top of the lua_State's stack.
    /// @remarks This requires two closures, no matter which type, to be present on the lua stack, else this function
    /// call will fail.
    static int hookfunction(lua_State *L);

    /// @brief Unhooks the function present at the top of the lua_State's stack.
    /// @remarks This requires a closures, no matter which type, to be present on the lua stack.
    static int unhookfunction(lua_State *L);

    /// @brief Wraps the closure at the top of the lua_State's stack into a C closure, no matter which type.
    /// @remarks This function will leave the resulting new C closure at the top of the lua stack!
    static int newcclosure(lua_State *L);

    /// @brief Wraps the closure at the top of the lua_State's stack into an L closure, no matter which type.
    /// @remarks This function will leave the resulting new L closure at the top of the lua stack!
    static int newlclosure(lua_State *L);

    /// @brief Clones the function present at the top of the stack. No matter which type of closure it is.
    /// @remarks This function will leave the resulting closure at the top of the lua stack!
    static int clonefunction(lua_State *L);
};
