//
// Created by Dottik on 15/9/2024.
//

#include "Debugger.hpp"

#include "Utilities.hpp"
#include "ldebug.h"

namespace RbxStu::Debugger {

    int breakpoint_thread(lua_State *L) {
        // TODO: fix.

        luaL_checktype(L, 1, lua_Type::LUA_TTHREAD);
        const auto oL = lua_tothread(L, 1);
        const bool crawlUntilLuaThread = luaL_optboolean(L, 2, true);

        auto currentCall = oL->ci;

        if (crawlUntilLuaThread) {
            while (currentCall != oL->base_ci && clvalue(currentCall->func)->isC) {
                currentCall--;
            }
        }

        const auto currentCallClosure = clvalue(currentCall->func);

        if (currentCallClosure->isC)
            luaL_argerror(L, 1, "the target thread's current call stack must point to a lua closure");

        if (currentCallClosure->l.p->lineinfo == nullptr)
            luaL_argerror(L, 1,
                          "the target thread's current call stack bytecode was compiled without debug information "
                          "(level 2). Cannot set breakpoint.");

        luaG_breakpoint(oL, currentCallClosure->l.p,
                        luaG_getline(currentCallClosure->l.p, pcRel(currentCall->savedpc, currentCallClosure->l.p)),
                        true);

        return 0;
    }

}; // namespace RbxStu::Debugger

std::string Debugger::GetLibraryName() { return "debugger"; }
luaL_Reg *Debugger::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"breakpoint_thread", RbxStu::Debugger::breakpoint_thread}, {nullptr, nullptr}};

    return reg;
}
