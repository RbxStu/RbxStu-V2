//
// Created by Dottik on 18/8/2024.
//

#include "EnvironmentManager.hpp"

#include <vector>

#include "Communication.hpp"
#include "Libraries/Debug.hpp"
#include "Libraries/Globals.hpp"
#include "Logger.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "Utilities.hpp"
#include "lapi.h"
#include "lstring.h"
#include "lua.h"

std::shared_ptr<EnvironmentManager> EnvironmentManager::pInstance;

std::string Library::GetLibraryName() {
    Logger::GetSingleton()->PrintError(RbxStu::EnvironmentManager,
                                       "ERROR! Library::GetLibraryName(...) is not implemented by the inheritor!");
    throw std::exception("Function not implemented by inheritor.");
}

luaL_Reg *Library::GetLibraryFunctions() {
    Logger::GetSingleton()->PrintError(RbxStu::EnvironmentManager,
                                       "ERROR! Library::GetLibraryFunctions(...) is not implemented by the inheritor!");
    throw std::exception("Function not implemented by inheritor.");
}

std::shared_ptr<EnvironmentManager> EnvironmentManager::GetSingleton() {
    if (EnvironmentManager::pInstance == nullptr)
        EnvironmentManager::pInstance = std::make_shared<EnvironmentManager>();
    return EnvironmentManager::pInstance;
}

static lua_CFunction __index_game_original = nullptr;
static lua_CFunction __namecall_game_original = nullptr;

static std::vector<std::string> blockedServices = {"linkingservice",    "browserservice",
                                                   "httprbxapiservice", "opencloudservice",
                                                   "messagebusservice", "omnirecommendationsservice",
                                                   "captureservice",    "corepackages"};

static std::vector<std::string> blockedFunctions = {"openvideosfolder",
                                                    "openscreenshotsfolder",
                                                    "getrobuxbalance",
                                                    "performpurchase", // Solves PerformPurchaseV2
                                                    "promptbundlepurchase",
                                                    "promptnativepurchase",
                                                    "promptproductpurchase",
                                                    "promptpurchase",
                                                    "promptgamepasspurchase",
                                                    "promptrobloxpurchase",
                                                    "promptthirdpartypurchase",
                                                    "publish",
                                                    "getnessageid",
                                                    "openbrowserwindow",
                                                    "opennativeoverlay",
                                                    "requestinternal",
                                                    "executejavascript",
                                                    "emithybridevent",
                                                    "addcorescriptlocal",
                                                    "httprequestasync",
                                                    "reportabuse", // Avoid bans. | Handles ReportAbuseV3
                                                    "savescriptprofilingdata",
                                                    "openurl",
                                                    "deletecapture",
                                                    "deletecapturesasync"};


void EnvironmentManager::PushEnvironment(_In_ lua_State *L) {
    const auto logger = Logger::GetSingleton();

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, "_G");
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, "_ENV");
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, "shared");

    for (const std::vector<Library *> libList = {new Debug{}, new Globals{}}; const auto &lib: libList) {
        try {
            const auto envGlobals = lib->GetLibraryFunctions();
            lua_newtable(L);
            luaL_register(L, nullptr, envGlobals);
            lua_setreadonly(L, -1, true);
            lua_setglobal(L, lib->GetLibraryName().c_str());

            lua_pushvalue(L, LUA_GLOBALSINDEX);
            luaL_register(L, nullptr, envGlobals);
            lua_pop(L, 1);

        } catch (const std::exception &ex) {
            logger->PrintError(RbxStu::EnvironmentManager,
                               std::format("Failed to initialize {} for RbxStu. Error from Lua: {}",
                                           lib->GetLibraryName(), ex.what()));
            throw;
        }

        delete lib;
    }

    logger->PrintInformation(RbxStu::EnvironmentManager,
                             "Installing meta method hooks (for security and extra behaviour)!");

    try {

        lua_getglobal(L, "game");
        // auto game = static_cast<void **>(lua_touserdata(L, -1));
        lua_getmetatable(L, -1);
        lua_rawgetfield(L, -1, "__index");
        auto __index = lua_toclosure(L, -1);
        __index_game_original = __index->c.f;
        __index->c.f = [](lua_State *L) -> int {
            if (lua_type(L, 2) != lua_Type::LUA_TSTRING)
                return __index_game_original(L);

            auto index = lua_tostring(L, 2);

            if (!Security::GetSingleton()->IsOurThread(L) || index == nullptr)
                return __index_game_original(L);

            const auto loweredIndex = Utilities::ToLower(index);
            printf("%s", loweredIndex);
            for (const auto &func: blockedFunctions) {
                if (loweredIndex.find(func) != std::string::npos) {
                    goto banned__index;
                }
            }

            for (const auto &func: blockedServices) {
                if (loweredIndex.find(func) != std::string::npos) {
                    goto banned__index;
                }
            }

            if (loweredIndex.find("getservice") != std::string::npos ||
                loweredIndex.find("findservice") != std::string::npos) {
                // getservice / findservice
                lua_pushcclosure(
                        L,
                        [](lua_State *L) -> int {
                            auto serviceName = luaL_checkstring(L, 2);
                            const auto svcName = Utilities::ToLower(serviceName);
                            for (const auto &func: blockedServices) {
                                if (svcName.find(func) != std::string::npos) {
                                    Logger::GetSingleton()->PrintWarning(
                                            RbxStu::Anonymous, std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                                           "BLACKLISTED SERVICE! SERVICE ACCESSED: {}",
                                                                           svcName));

                                    if (Communication::GetSingleton()->IsUnsafeMode())
                                        break;

                                    luaL_errorL(L, "This service has been blocked for security");
                                }
                            }
                            lua_pushvalue(L, 1);
                            lua_pushstring(L, L->ci->func->value.gc->cl.c.upvals[0].value.gc->ts.data);
                            __index_game_original(L);
                            lua_pushvalue(L, 1);
                            lua_pushvalue(L, 2);
                            if (const auto err = lua_pcall(L, 2, 1, 0);
                                err == LUA_ERRRUN || err == LUA_ERRMEM || err == LUA_ERRERR)
                                lua_error(L);
                            return 1;
                        },
                        nullptr, 1);
                const auto cl = lua_toclosure(L, -1);
                cl->c.upvals[0].tt = lua_Type::LUA_TSTRING;
                cl->c.upvals[0].value.gc->ts = *luaS_newlstr(L, index, strlen(index));

                return 1;
            }


            if (loweredIndex.find("httpget") != std::string::npos ||
                loweredIndex.find("httpgetasync") != std::string::npos) {
                lua_getglobal(L, "httpget");
                return 1;
            }

            if (loweredIndex.find("getobjects") != std::string::npos) {
                lua_getglobal(L, "GetObjects");
                return 1;
            }


            if (false) {
            banned__index:
                Logger::GetSingleton()->PrintWarning(RbxStu::Anonymous,
                                                     std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                                 "BLACKLISTED FUNCTION/SERVICE! FUNCTION ACCESSED: {}",
                                                                 index));
                if (Communication::GetSingleton()->IsUnsafeMode())
                    return __index_game_original(L);

                luaL_errorL(L, "This service/function has been blocked for security");
            }


            return __index_game_original(L);
        };
        lua_pop(L, 1);
        lua_rawgetfield(L, -1, "__namecall");
        auto __namecall = lua_toclosure(L, -1);
        __namecall_game_original = __namecall->c.f;
        __namecall->c.f = [](lua_State *L) -> int {
            if (lua_type(L, 2) != lua_Type::LUA_TSTRING)
                return __namecall_game_original(L);

            auto namecall = L->namecall->data;

            if (!Security::GetSingleton()->IsOurThread(L) || namecall == nullptr)
                return __namecall_game_original(L);

            const auto loweredNamecall = Utilities::ToLower(namecall);
            for (const auto &func: blockedFunctions) {
                if (loweredNamecall.find(func) != std::string::npos) {
                    goto banned__namecall;
                }
            }

            for (const auto &func: blockedServices) {
                if (loweredNamecall.find(func) != std::string::npos) {
                    goto banned__namecall;
                }
            }

            if (false) {
            banned__namecall:
                Logger::GetSingleton()->PrintWarning(RbxStu::Anonymous,
                                                     std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                                 "BLACKLISTED FUNCTION/SERVICE! FUNCTION ACCESSED: {}",
                                                                 namecall));
                if (Communication::GetSingleton()->IsUnsafeMode())
                    return __namecall_game_original(L);

                luaL_errorL(L, "This service/function has been blocked for security");
            }

            if (loweredNamecall.find("getservice") != std::string::npos ||
                loweredNamecall.find("findservice") != std::string::npos) {
                // getservice / findservice
                auto serviceName = luaL_checkstring(L, 2);
                const auto svcName = Utilities::ToLower(serviceName);
                for (const auto &func: blockedServices) {
                    if (svcName.find(func) != std::string::npos) {
                        Logger::GetSingleton()->PrintWarning(RbxStu::Anonymous,
                                                             std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                                         "BLACKLISTED SERVICE! SERVICE ACCESSED: {}",
                                                                         svcName));

                        if (Communication::GetSingleton()->IsUnsafeMode())
                            break;

                        luaL_errorL(L, "This service has been blocked for security");
                    }
                }
                __namecall_game_original(L);

                return 1;
            }

            if (loweredNamecall.find("httpget") != std::string::npos ||
                loweredNamecall.find("httpgetasync") != std::string::npos) {
                luaL_checkstring(L, 2);
                lua_getglobal(L, "httpget");
                lua_pushvalue(L, 2);
                const auto err = lua_pcall(L, 1, 1, 0);
                if (strcmp(lua_tostring(L, -1), "attempt to yield across metamethod/C-call boundary") == 0)
                    return lua_yield(L, 1);

                if (err == LUA_ERRRUN || err == LUA_ERRMEM || err == LUA_ERRERR)
                    lua_error(L);

                return 1;
            }

            if (loweredNamecall.find("getobjects") != std::string::npos) {
                lua_pushvalue(L, 2);
                lua_getglobal(L, "GetObjects");
                const auto err = lua_pcall(L, 1, 1, 0);
                if (err == LUA_ERRRUN || err == LUA_ERRMEM || err == LUA_ERRERR)
                    lua_error(L);

                if (err == LUA_YIELD)
                    return lua_yield(L, 1);
                return 1;
            }

            return __namecall_game_original(L);
        };

        lua_pop(L, lua_gettop(L));
    } catch (const std::exception &ex) {
        logger->PrintError(RbxStu::EnvironmentManager,
                           std::format("Failed to initialize RbxStu Environment. Error from Lua: {}", ex.what()));
    }
}
