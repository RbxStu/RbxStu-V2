//
// Created by Pixeluted on 22/08/2024.
//

#include "Closures.hpp"

#include "ClosureManager.hpp"
#include "Communication/Communication.hpp"
#include "Logger.hpp"
#include "Luau/CodeGen.h"
#include "Luau/Compiler.h"
#include "Security.hpp"

namespace RbxStu {
    namespace Closures {
        int checkcaller(lua_State *L) {
            // Our check caller implementation, is, in fact, quite simple!
            // We leave the 63d bit on the capabilities set. Then we retrieve it AND it, receiving the expected, if it
            // doesn't match, then, we are 100% not in our thread.
            lua_pushboolean(L, Security::GetSingleton()->IsOurThread(L));
            return 1;
        }

        int getcallingscript(lua_State *L) {
            try {
                lua_getglobal(L, "script");
            } catch (...) {
                lua_pushnil(L);
            }

            return 1;
        }

        int islclosure(lua_State *L) {
            luaL_checktype(L, 1, lua_Type::LUA_TFUNCTION);

            lua_pushboolean(L, !lua_iscfunction(L, 1));
            return 1;
        }

        int iscclosure(lua_State *L) {
            luaL_checktype(L, 1, lua_Type::LUA_TFUNCTION);

            lua_pushboolean(L, lua_iscfunction(L, 1));
            return 1;
        }

        int isourclosure(lua_State *L) {
            luaL_checktype(L, 1, lua_Type::LUA_TFUNCTION);

            if (const auto pClosure = lua_toclosure(L, 1); pClosure->isC) {
                lua_pushboolean(L, pClosure->c.debugname == nullptr ||
                                           ClosureManager::GetSingleton()->IsWrappedCClosure(pClosure));
            } else {
                lua_pushboolean(L, pClosure->l.p->linedefined == -1);
            }

            return 1;
        }

        int loadstring(lua_State *L) {
            const auto luauCode = luaL_checkstring(L, 1);
            const auto chunkName = luaL_optstring(L, 2, "RbxStuV2_LoadString");
            constexpr auto compileOpts = Luau::CompileOptions{1, 2};
            const auto bytecode = Luau::compile(luauCode, compileOpts);

            if (luau_load(L, chunkName, bytecode.c_str(), bytecode.size(), 0) != lua_Status::LUA_OK) {
                lua_pushnil(L);
                lua_pushvalue(L, -2);
                return 2;
            }

            if (Communication::GetSingleton()->IsCodeGenerationEnabled()) {
                const Luau::CodeGen::CompilationOptions opts{0};
                Logger::GetSingleton()->PrintInformation(
                        RbxStu::Anonymous, "Native Code Generation is enabled! Compiling Luau Bytecode -> Native");
                Luau::CodeGen::compile(L, -1, opts);
            }


            Security::GetSingleton()->SetLuaClosureSecurity(lua_toclosure(L, -1), 8);
            lua_setsafeenv(L, LUA_GLOBALSINDEX, false); // env is not safe anymore.
            return 1;
        }
    } // namespace Closures
} // namespace RbxStu

std::string Closures::GetLibraryName() { return "closures"; }

luaL_Reg *Closures::GetLibraryFunctions() {
    const auto reg = new luaL_Reg[]{{"checkcaller", RbxStu::Closures::checkcaller},
                                    {"clonefunction", ClosureManager::clonefunction},
                                    {"getcallingscript", RbxStu::Closures::getcallingscript},

                                    {"hookfunction", ClosureManager::hookfunction},
                                    {"replaceclosure", ClosureManager::hookfunction},
                                    {"unhookfunction", ClosureManager::unhookfunction},
                                    {"restorefunction", ClosureManager::unhookfunction},

                                    {"newlclosure", ClosureManager::newlclosure},
                                    {"newcclosure", ClosureManager::newcclosure},
                                    {"iscclosure", RbxStu::Closures::iscclosure},
                                    {"islclosure", RbxStu::Closures::islclosure},

                                    {"isourclosure", RbxStu::Closures::isourclosure},
                                    {"checkclosure", RbxStu::Closures::isourclosure},
                                    {"isexecutorclosure", RbxStu::Closures::isourclosure},

                                    {"loadstring", RbxStu::Closures::loadstring},

                                    {nullptr, nullptr}};

    return reg;
}
