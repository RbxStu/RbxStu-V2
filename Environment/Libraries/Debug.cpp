//
// Created by Dottik on 19/8/2024.
//

#include "Debug.hpp"
#include <iostream>
#include "Luau/BytecodeBuilder.h"
#include "Utilities.hpp"
#include "lapi.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lua.h"
#include "lualib.h"
namespace RbxStu {
    namespace Debug {
        int debug_getconstants(lua_State *L) {
            luaL_checkany(L, 1);

            if (lua_isfunction(L, 1) == false && !lua_isnumber(L, 1)) {
                luaL_typeerror(L, 1, "Expected function or number for argument #1");
            }

            if (lua_isnumber(L, 1)) {
                lua_Debug dbgInfo{};

                if (const int num = lua_tonumber(L, 1); !lua_getinfo(L, num, "f", &dbgInfo))
                    luaL_error(L, "Failed to get information");

                if (lua_iscfunction(L, -1))
                    luaL_argerror(L, 1, "stack points to a C closure, Lua function expected.");
            } else {
                lua_pushvalue(L, 1); // Push copy of stack item.

                if (lua_iscfunction(L, 1))
                    luaL_argerror(L, 1, "Cannot get constants on a C closure.");
            }
            const auto *pClosure = lua_toclosure(L, -1);
            auto constCount = pClosure->l.p->sizek;
            auto consts = pClosure->l.p->k;

            lua_newtable(L);

            for (int i = 0; i < constCount; i++) {
                TValue *tval = &(consts[i]);

                if (tval->tt == LUA_TFUNCTION) {
                    lua_pushnil(L);
                } else {
                    L->top->value = tval->value;
                    L->top->tt = tval->tt;
                    L->top++;
                }

                lua_rawseti(L, -2, (i + 1));
            }

            return 1;
        }

        int debug_setconstant(lua_State *L) {
            if (lua_isfunction(L, 1) == false && !lua_isnumber(L, 1))
                luaL_typeerror(L, 1, "function or level expected");

            const int index = luaL_checkinteger(L, 2);

            luaL_checkany(L, 3);

            if (lua_isnumber(L, 1)) {
                lua_Debug ar;

                if (!lua_getinfo(L, lua_tonumber(L, 1), "f", &ar))
                    luaL_error(L, "level out of range");

                if (lua_iscfunction(L, -1))
                    luaL_argerror(L, 1, "stack points to a C closure, Lua closure expected.");
            } else {
                lua_pushvalue(L, 1);

                if (lua_iscfunction(L, -1))
                    luaL_argerror(L, 1, "Cannot set constants on a C closure");
            }

            const auto cl = clvalue(luaA_toobject(L, -1));
            const auto *p = cl->l.p;
            auto *k = p->k;

            if (index < 1)
                luaL_argerror(L, 2, "constant index starts at 1");

            if (index > p->sizek)
                luaL_argerror(L, 2, "constant index out of range");

            const auto constant = &k[index - 1];

            if (constant->tt == LUA_TFUNCTION)
                return 0;

            const TValue *new_t = luaA_toobject(L, 3);
            constant->tt = new_t->tt;
            constant->value = new_t->value;

            return 0;
        }

        int debug_getconstant(lua_State *L) {
            luaL_checkany(L, 2);

            if (lua_isfunction(L, 1) == false && !lua_isnumber(L, 1)) {
                luaL_typeerror(L, 1, "Expected function or number for argument #1");
            }

            const int dbgIndx = luaL_checkinteger(L, 2);

            if (lua_isnumber(L, 1)) {
                lua_Debug dbgInfo{};

                int num = lua_tonumber(L, 1);

                if (!lua_getinfo(L, num, "f", &dbgInfo)) {
                    luaL_error(L, "Failed to get information");
                }

                if (lua_iscfunction(L, -1)) {
                    luaL_argerror(L, 1, "stack points to a C closure, Lua function expected.");
                }
            } else {
                lua_pushvalue(L, 1); // Push copy of stack item.

                if (lua_iscfunction(L, -1)) {
                    luaL_argerror(L, 1, "Lua function expected.");
                }
            }
            const auto *pClosure = lua_toclosure(L, -1);
            const auto constants = pClosure->l.p->k;

            if (dbgIndx < 1)
                luaL_argerror(L, 2, "constant index starts at 1");

            if (dbgIndx > pClosure->l.p->sizek)
                luaL_argerror(L, 2, "constant index is out of range");

            if (const auto tValue = &constants[dbgIndx - 1]; tValue->tt == LUA_TFUNCTION) {
                lua_pushnil(L);
            } else {
                L->top->tt = tValue->tt;
                L->top->value = tValue->value;
                L->top++;
            }

            return 1;
        }

        int debug_getinfo(lua_State *L) {
            luaL_checkany(L, 1);
            auto infoLevel = 0;

            if (lua_isnumber(L, 1)) {
                infoLevel = lua_tointeger(L, 1);
                luaL_argcheck(L, infoLevel >= 0, 1, "level cannot be negative");
            } else if (lua_isfunction(L, 1)) {
                infoLevel = -lua_gettop(L);
            } else {
                luaL_argerror(L, 1, "function or level expected");
            }

            lua_Debug lDbg{};

            if (!lua_getinfo(L, infoLevel, "fulasn", &lDbg))
                luaL_argerror(L, 1, "invalid level");

            lua_newtable(L);

            lua_pushstring(L, lDbg.source);
            lua_setfield(L, -2, "source");

            lua_pushstring(L, lDbg.short_src);
            lua_setfield(L, -2, "short_src");

            lua_pushvalue(L, 1);
            lua_setfield(L, -2, "func");

            lua_pushstring(L, lDbg.what);
            lua_setfield(L, -2, "what");

            lua_pushinteger(L, lDbg.currentline);
            lua_setfield(L, -2, "currentline");

            lua_pushstring(L, lDbg.name);
            lua_setfield(L, -2, "name");

            lua_pushinteger(L, lDbg.nupvals);
            lua_setfield(L, -2, "nups");

            lua_pushinteger(L, lDbg.nparams);
            lua_setfield(L, -2, "numparams");

            lua_pushinteger(L, lDbg.isvararg);
            lua_setfield(L, -2, "is_vararg");

            return 1;
        }

        __forceinline lua_State *getthread(_In_ lua_State *L, _Inout_ int *arg) {
            if (lua_isthread(L, 1)) {
                *arg = 1;
                return lua_tothread(L, 1);
            }

            *arg = 0;
            return L;
        }

        int debug_traceback(lua_State *L) {
            int arg{0};
            lua_State *L1 = getthread(L, &arg);
            const char *msg = luaL_optstring(L, arg + 1, NULL);
            const int level = luaL_optinteger(L, arg + 2, (L == L1) ? 1 : 0);
            luaL_argcheck(L, level >= 0, arg + 2, "level can't be negative");

            luaL_Strbuf buf;
            luaL_buffinit(L, &buf);

            if (msg) {
                luaL_addstring(&buf, msg);
                luaL_addstring(&buf, "\n");
            }

            lua_Debug ar;
            for (int i = level; lua_getinfo(L1, i, "sln", &ar); ++i) {
                if (strcmp(ar.what, "C") == 0)
                    continue;

                if (ar.source)
                    luaL_addstring(&buf, ar.short_src);

                if (ar.currentline > 0) {
                    char line[32]; // manual conversion for performance
                    char *lineend = line + sizeof(line);
                    char *lineptr = lineend;
                    for (unsigned int r = ar.currentline; r > 0; r /= 10)
                        *--lineptr = '0' + (r % 10);

                    luaL_addchar(&buf, ':');
                    luaL_addlstring(&buf, lineptr, lineend - lineptr);
                }

                if (ar.name) {
                    luaL_addstring(&buf, " function ");
                    luaL_addstring(&buf, ar.name);
                }

                luaL_addchar(&buf, '\n');
            }

            luaL_pushresult(&buf);
            return 1;
        }

        int debug_info(lua_State *L) {
            int arg{0};
            lua_State *L1 = getthread(L, &arg);
            int l1top{0};

            // if L1 != L, L1 can be in any state, and therefore there are no guarantees about its stack space
            if (L != L1) {
                // for 'f' option, we reserve one slot and we also record the stack top
                lua_rawcheckstack(L1, 1);

                l1top = lua_gettop(L1);
            }

            int level;
            if (lua_isnumber(L, arg + 1)) {
                level = (int) lua_tointeger(L, arg + 1);
                luaL_argcheck(L, level >= 0, arg + 1, "level can't be negative");
            } else if (arg == 0 && lua_isfunction(L, 1)) {
                // convert absolute index to relative index
                level = -lua_gettop(L);
            } else
                luaL_argerror(L, arg + 1, "function or level expected");

            const char *options = luaL_checkstring(L, arg + 2);

            lua_Debug ar;
            if (!lua_getinfo(L1, level, options, &ar))
                return 0;

            int results = 0;
            bool occurs[26] = {};

            for (const char *it = options; *it; ++it) {
                if (static_cast<unsigned int>(*it - 'a') < 26) {
                    if (occurs[*it - 'a']) {
                        // restore stack state of another thread as 'f' option might not have been visited yet
                        if (L != L1)
                            lua_settop(L1, l1top);

                        luaL_argerror(L, arg + 2, "duplicate option");
                    }
                    occurs[*it - 'a'] = true;
                }

                switch (*it) {
                    case 's':
                        lua_pushstring(L, ar.short_src);
                        results++;
                        break;

                    case 'l':
                        lua_pushinteger(L, ar.currentline);
                        results++;
                        break;

                    case 'n':
                        lua_pushstring(L, ar.name ? ar.name : "");
                        results++;
                        break;

                    case 'f':
                        if (L1 == L)
                            lua_pushvalue(L, -1 - results); // function is right before results
                        else
                            lua_xmove(L1, L, 1); // function is at top of L1
                        results++;
                        break;

                    case 'a':
                        lua_pushinteger(L, ar.nparams);
                        lua_pushboolean(L, ar.isvararg);
                        results += 2;
                        break;

                    default:
                        luaL_argerror(L, arg + 2, "invalid option");
                }
            }

            return results;
        }

        int debug_getproto(lua_State *L) {
            luaL_checktype(L, 2, LUA_TNUMBER);

            const bool active = luaL_optboolean(L, 3, false);

            if (!active)
                luaL_argerror(L, 3, "inactive closures are not implemented, debug.getproto will not run.");


            if (lua_isnumber(L, 1) == false && lua_isfunction(L, 1) == false) {
                luaL_argerror(L, 1, "function or level expected");
            }

            if (lua_isnumber(L, 1)) {
                const int level = lua_tointeger(L, 1);

                if (level >= L->ci - L->base_ci || level < 0) {
                    luaL_argerror(L, 1, "stack out of range.");
                }

                lua_Debug ar;
                if (!lua_getinfo(L, level, "f", &ar)) {
                    luaL_error(L, "Failed to get information");
                }
            } else {
                luaL_checktype(L, 1, LUA_TFUNCTION);
                lua_pushvalue(L, 1);
            }

            if (lua_toclosure(L, -1)->isC)
                luaL_argerrorL(L, 1, "level/func must point/be a Lua function");

            const auto closure = clvalue(luaA_toobject(L, -1));

            const auto index = lua_tointeger(L, 2);

            if (index < 1 || index > closure->l.p->sizep) {
                luaL_argerror(L, 2, "proto index out of range");
            }

            auto proto = closure->l.p->p[index - 1];

            setclvalue(L, L->top, luaF_newLclosure(L, proto->nups, closure->env, proto));
            L->top++;

            return 1;
        }

        int debug_getprotos(lua_State *L) {
            luaL_checkany(L, 1);

            if (lua_isnumber(L, 1) == false && lua_isfunction(L, 1) == false) {
                luaL_argerror(L, 1, "function or level expected");
            }

            if (lua_isnumber(L, 1)) {
                const int level = lua_tointeger(L, 1);

                if (level >= L->ci - L->base_ci || level < 0) {
                    luaL_argerror(L, 1, "stack out of range.");
                }

                lua_Debug ar;
                lua_getinfo(L, level, "f", &ar);

                if (clvalue(reinterpret_cast<CallInfo *>(L->ci - level)->func)->isC) {
                    luaL_argerror(L, 1, "Stack level is a C closure. Lua closure expected.");
                }
            } else {
                luaL_checktype(L, 1, LUA_TFUNCTION);
                if (const auto cl = clvalue(luaA_toobject(L, 1)); cl->isC) {
                    luaL_argerror(L, 1, "stack points to a C closure, Lua closure expected!");
                }

                lua_pushvalue(L, 1);
            }
            const auto *cl = lua_toclosure(L, -1);

            lua_newtable(L);

            const auto *mProto = cl->l.p;

            for (int i = 0; i < mProto->sizep; i++) {
                Proto *proto = mProto->p[i];
                Closure *lclosure = luaF_newLclosure(L, proto->nups, cl->env, proto);

                setclvalue(L, L->top, lclosure);
                L->top++;

                lua_rawseti(L, -2, i + 1);
            }

            return 1;
        }

        int debug_setstack(lua_State *L) {
            luaL_checktype(L, 1, LUA_TNUMBER);
            luaL_checktype(L, 2, LUA_TNUMBER);
            luaL_checkany(L, 3);

            const auto level = lua_tointeger(L, 1);
            const auto index = lua_tointeger(L, 2);

            if (level >= L->ci - L->base_ci || level < 0)
                luaL_argerror(L, 1, "level out of range");

            const auto frame = L->ci - level;
            const auto top = frame->top - frame->base;

            if (clvalue(frame->func)->isC)
                luaL_argerror(L, 1, "level points to a C closure, Lua closure expected!");

            if (index < 1 || index > top)
                luaL_argerror(L, 2, "stack index out of range");

            if (frame->base[index - 1].tt != lua_type(L, 3))
                luaL_argerror(L, 2, "type on the stack is different than that you are trying to set!");

            setobj2s(L, &frame->base[index - 1], luaA_toobject(L, 3));
            return 0;
        }

        int debug_getstack(lua_State *L) {
            luaL_checktype(L, 1, LUA_TNUMBER);

            const auto level = lua_tointeger(L, 1);
            const auto index = luaL_optinteger(L, 2, -1);

            if (level >= L->ci - L->base_ci || level < 0)
                luaL_argerror(L, 1, "level out of range");

            const auto frame = L->ci - level;
            const std::size_t stackFrameSize = (frame->top - frame->base);

            if (clvalue(frame->func)->isC)
                luaL_argerror(L, 1, "level points to a C closure, Lua closure expected!");

            if (index == -1) {
                lua_newtable(L);
                for (int i = 0; i < stackFrameSize; i++) {
                    setobj2s(L, L->top, &frame->base[i]);
                    L->top++;

                    lua_rawseti(L, -2, i + 1);
                }
            } else {
                if (index < 1 || index > stackFrameSize)
                    luaL_argerror(L, 2, "index out of range");

                setobj2s(L, L->top, &frame->base[index - 1]);
                L->top++;
            }

            return 1;
        }

        int debug_setupvalue(lua_State *L) {
            if (lua_isfunction(L, 1) == false && lua_isnumber(L, 1) == false)
                luaL_typeerror(L, 1, "function or level expected");

            if (lua_iscfunction(L, 1))
                luaL_argerrorL(L, 1, "cannot set upvalues of C functions");

            const int index = luaL_checkinteger(L, 2);
            luaL_checkany(L, 3);


            if (lua_isnumber(L, 1)) {
                lua_Debug ar;

                if (!lua_getinfo(L, lua_tonumber(L, 1), "f", &ar))
                    luaL_error(L, "level out of range");
            } else {
                lua_pushvalue(L, 1);
            }

            auto *cl = clvalue(luaA_toobject(L, -1));
            const TValue *value = luaA_toobject(L, 3);
            auto *upvalue_table = static_cast<TValue *>(nullptr);

            if (!cl->isC)
                upvalue_table = cl->l.uprefs;

            if (index < 1)
                luaL_argerror(L, 2, "upvalue index starts at 1");

            if (index > cl->nupvalues)
                luaL_argerror(L, 2, "upvalue index out of range");

            TValue *upvalue = (&upvalue_table[index - 1]);

            upvalue->value = value->value;

            upvalue->tt = value->tt;

            luaC_barrier(L, cl, value);
            lua_pushboolean(L, true);

            return 1;
        }

        int debug_getupvalue(lua_State *L) {
            luaL_checktype(L, 2, LUA_TNUMBER); // index

            if (lua_isfunction(L, 1) == false && lua_isnumber(L, 1) == false)
                luaL_typeerror(L, 1, "function or level expected");

            if (lua_isnumber(L, 1)) {
                lua_Debug ar;

                if (!lua_getinfo(L, lua_tonumber(L, 1), "f", &ar))
                    luaG_runerror(L, "level out of range");
            } else {
                lua_pushvalue(L, 1);
            }

            const int index = luaL_checkinteger(L, 2);

            const auto *cl = clvalue(luaA_toobject(L, -1));
            const auto *upvalueTable = static_cast<TValue *>(nullptr);

            if (!cl->isC)
                upvalueTable = cl->l.uprefs;
            else if (cl->isC)
                upvalueTable = cl->c.upvals;

            if (!index)
                luaL_argerror(L, 2, "upvalue index starts at 1");

            if (index > cl->nupvalues)
                luaL_argerror(L, 2, "upvalue index is out of range");

            const auto *upval = &upvalueTable[index - 1];
            auto *top = L->top;

            top->value = upval->value;
            top->tt = upval->tt;
            L->top++;

            return 1;
        }

        int debug_getupvalues(lua_State *L) {
            if (lua_isfunction(L, 1) == false && lua_isnumber(L, 1) == false)
                luaL_typeerror(L, 1, "function or level expected");

            if (lua_isnumber(L, 1)) {
                lua_Debug ar;

                if (!lua_getinfo(L, lua_tonumber(L, 1), "f", &ar))
                    luaG_runerror(L, "level out of range");
            } else {
                lua_pushvalue(L, 1);
            }

            const auto *cl = clvalue(luaA_toobject(L, -1));
            const auto *upvalueTable = static_cast<TValue *>(nullptr);

            lua_newtable(L);


            if (!cl->isC)
                upvalueTable = cl->l.uprefs;
            else if (cl->isC)
                upvalueTable = cl->c.upvals;

            for (int i = 0; i < cl->nupvalues; i++) {
                const auto *upval = (&upvalueTable[i]);
                auto *top = L->top;

                top->value = upval->value;
                top->tt = upval->tt;
                L->top++;

                lua_rawseti(L, -2, (i + 1));
            }

            return 1;
        }

        int getregistry(lua_State *L) {
            lua_pushvalue(L, LUA_REGISTRYINDEX);
            return 1;
        }
    } // namespace Debug
} // namespace RbxStu

std::string Debug::GetLibraryName() { return "debug"; }


luaL_Reg *Debug::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{
            {"setconstant", RbxStu::Debug::debug_setconstant},
            {"getconstant", RbxStu::Debug::debug_getconstant},
            {"getconstants", RbxStu::Debug::debug_getconstants},

            {"getinfo", RbxStu::Debug::debug_getinfo},
            {"info", RbxStu::Debug::debug_info},
            {"traceback", RbxStu::Debug::debug_traceback},

            {"getproto", RbxStu::Debug::debug_getproto},
            {"getprotos", RbxStu::Debug::debug_getprotos},

            {"getstack", RbxStu::Debug::debug_getstack},
            {"setstack", RbxStu::Debug::debug_setstack},

            {"setupvalue", RbxStu::Debug::debug_setupvalue},

            {"getupvalue", RbxStu::Debug::debug_getupvalue},
            {"getupvalues", RbxStu::Debug::debug_getupvalues},

            {"getregistry", RbxStu::Debug::getregistry},

            {nullptr, nullptr},
    };

    return reg;
}
