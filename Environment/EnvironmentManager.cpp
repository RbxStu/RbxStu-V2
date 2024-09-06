//
// Created by Dottik on 18/8/2024.
//

#include "EnvironmentManager.hpp"

#include <cstdlib>
#include <vector>

#include "Communication/Communication.hpp"
#include "Libraries/Cache.hpp"
#include "Libraries/Closures.hpp"
#include "Libraries/Console.hpp"
#include "Libraries/Debug.hpp"
#include "Libraries/Filesystem.hpp"
#include "Libraries/Globals.hpp"
#include "Libraries/Http.hpp"
#include "Libraries/Input.hpp"
#include "Libraries/Instance.hpp"
#include "Libraries/Metatable.hpp"
#include "Libraries/Misc.hpp"
#include "Libraries/Script.hpp"
#include "Libraries/WebSocket.hpp"
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

static std::vector<std::string> blockedServices = {"linkingservice",
                                                   "browserservice",
                                                   "httprbxapiservice",
                                                   "opencloudservice",
                                                   "messagebusservice",
                                                   "omnirecommendationsservice",
                                                   "captureservice",
                                                   "corepackages",
                                                   "animationfromvideocreatorservice",
                                                   "safetyservice",
                                                   "appupdateservice",
                                                   "ugcvalidationservice",
                                                   "accountservice",
                                                   "analyticsservice",
                                                   "ixpservice",
                                                   "commerceservice",
                                                   "sessionservice",
                                                   "studioservice",
                                                   "platformcloudstorageservice",
                                                   "startpageservice",
                                                   "scripteditorservice"};

static std::vector<std::string> blockedFunctions = {
        "openvideosfolder", "openscreenshotsfolder", "getrobuxbalance",
        "performpurchase", // Solves PerformPurchaseV2
        "promptbundlepurchase", "promptnativepurchase", "promptproductpurchase", "promptpurchase",
        "promptgamepasspurchase", "promptrobloxpurchase", "promptthirdpartypurchase", "publish", "getnessageid",
        "openbrowserwindow", "opennativeoverlay", "requestinternal", "executejavascript", "emithybridevent",
        "addcorescriptlocal", "httprequestasync",
        "reportabuse", // Avoid bans. | Handles ReportAbuseV3
        "savescriptprofilingdata", "openurl", "openinbrowser", "deletecapture", "deletecapturesasync",
        "promptbulkpurchase", "performbulkpurchase",
        "performsubscriptionpurchase", // Solves PerformSubscriptionPurchaseV2
        "promptcollectiblespurchase", "promptnativepurchasewithlocalplayer", "promptpremiumpurchase",
        "promptsubscriptionpurchase", "getusersubscriptionpaymenthistoryasync", "broadcastnotification",
        "setpurchasepromptisshown", "addcorescriptlocal", "savescriptprofilingdata", "installplugin", "importfile",
        "requestclose", "plugininstalled", "publishtoroblox", "setbaseurl", "requestinternal",
        // "getasync",
        // "postasync",
        "openscreenshotsfolder", "openvideosfolder", "takescreenshot", "togglerecording", "startplaying", "getsecret",
        "requestasync", "sendrobloxevent",
        "startsessionwithpath" // StartSessionWithPathAsync
};

void EnvironmentManager::SetFunctionBlocked(const std::string &functionName, bool status) {
    const auto loweredFunctionName = Utilities::ToLower(functionName);
    if (status) {
        for (auto &service: blockedFunctions) {
            if (service == loweredFunctionName) {
                return; // Already blocked.
            }
        }
        blockedFunctions.push_back(loweredFunctionName);
    } else {
        for (int i = 0; i < blockedFunctions.size(); ++i) {
            if (blockedFunctions[i] == loweredFunctionName) {
                blockedFunctions.erase(blockedFunctions.begin() + i);
                return; // Function was found and unblocked
            }
        }
    }
}

void EnvironmentManager::SetServiceBlocked(const std::string &serviceName, bool status) {
    const auto loweredServiceName = Utilities::ToLower(serviceName);
    if (status) {
        for (auto &service: blockedServices) {
            if (service == loweredServiceName) {
                return; // Already blocked.
            }
        }
        blockedServices.push_back(loweredServiceName);
    } else {
        for (int i = 0; i < blockedServices.size(); ++i) {
            if (blockedServices[i] == loweredServiceName) {
                blockedServices.erase(blockedServices.begin() + i);
                return; // Service was found and unblocked
            }
        }
    }
}

void EnvironmentManager::PushEnvironment(_In_ lua_State *L) {
    const auto logger = Logger::GetSingleton();

    // Don't replace renv globals.
    // lua_pushvalue(L, LUA_GLOBALSINDEX);
    // lua_setglobal(L, "_G");
    // lua_pushvalue(L, LUA_GLOBALSINDEX);
    // lua_setglobal(L, "shared");
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, "_ENV");

    for (const std::vector<Library *> libList = {new Debug{}, new Globals{}, new Filesystem(), new Closures(),
                                                 new Metatable(), new Cache(), new Console(), new Script(), new Misc(),
                                                 new Instance(), new Input(), new Http() /*, new Websocket()*/};
         const auto &lib: libList) {
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

            if (!Communication::GetSingleton()->CanAccessScriptSource() &&
                loweredIndex.find("source") != std::string::npos) {
                Utilities::checkInstance(L, 1, "LuaSourceContainer");
                lua_pushstring(L, "");
                return 1;
            }

            if (loweredIndex.find("getservice") != std::string::npos ||
                loweredIndex.find("findservice") != std::string::npos) {
                // getservice / findservice
                lua_pushcclosure(
                        L,
                        [](lua_State *L) -> int {
                            luaL_checktype(L, 1, lua_Type::LUA_TUSERDATA);
                            auto dataModel = lua_topointer(L, 1);
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
                            lua_pop(L, 1);
                            L->top->value.p = L->ci->func->value.p;
                            L->top->tt = lua_Type::LUA_TFUNCTION;
                            L->top++;
                            lua_getupvalue(L, -1, 1);
                            lua_remove(L, 2);
                            __index_game_original(L);
                            lua_pushvalue(L, 1);
                            lua_pushstring(L, serviceName);
                            lua_pcall(L, 2, 1, 0);
                            return 1;
                        },
                        nullptr, 1);
                const auto cl = lua_toclosure(L, -1);
                lua_pushstring(L, index);
                lua_setupvalue(L, -2, 1);
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
                if (lua_type(L, -1) == lua_Type::LUA_TSTRING &&
                    strcmp(lua_tostring(L, -1), "attempt to yield across metamethod/C-call boundary") == 0)
                    return lua_yield(L, 1);

                if (err == LUA_ERRRUN || err == LUA_ERRMEM || err == LUA_ERRERR)
                    lua_error(L);

                return 1;
            }

            if (loweredNamecall.find("getobjects") != std::string::npos) {
                lua_getglobal(L, "GetObjects");
                lua_pushvalue(L, 2);
                const auto err = lua_pcall(L, 1, 1, 0);
                if (lua_type(L, -1) == lua_Type::LUA_TSTRING &&
                    strcmp(lua_tostring(L, -1), "attempt to yield across metamethod/C-call boundary") == 0)
                    return lua_yield(L, 1);
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

    Scheduler::GetSingleton()->ScheduleJob(SchedulerJob(
            R"(
local insertservice_LoadLocalAsset = clonefunction(cloneref(game.GetService(game, "InsertService")).LoadLocalAsset)
local insertservice_LoadAsset = clonefunction(cloneref(game.GetService(game, "InsertService")).LoadAsset)
local table_insert = clonefunction(table.insert)
local getreg = clonefunction(getreg)
local string_gsub = clonefunction(string.gsub)
local typeof = clonefunction(typeof)
local error = clonefunction(error)
local getfenv = clonefunction(getfenv)
local getgc = clonefunction(getgc)
local Instance_new = clonefunction(Instance.new)
local rawget = clonefunction(rawget)
local pcall = clonefunction(pcall)
local setidentity = clonefunction(setidentity)
local getidentity = clonefunction(getidentity)

local instanceList = nil
local getInstanceList = newcclosure(function()
	if instanceList ~= nil then
		return instanceList
	end
	local tmp = Instance_new("Part")
	for idx, val in getreg() do
		if typeof(val) == "table" and rawget(val, "__mode") == "kvs" then
			for idx_, inst in val do
				if inst == tmp then
					tmp:Destroy()
					instanceList = val
					return instanceList -- Instance list
				end
			end
		end
	end
	tmp:Destroy()
	return {}
end)

getgenv().getnilinstances = newcclosure(function()
	local instances = {}
	for i, v in getInstanceList() do
		if typeof(v) == "Instance" and not v.Parent then
			table_insert(instances, v)
		end
	end
	return instances
end)

getgenv().getinstances = newcclosure(function()
	local instances = {}
	for i, v in getInstanceList() do
		if typeof(v) == "Instance" then
			table_insert(instances, v)
		end
	end
	return instances
end)

getgenv().getscripts = newcclosure(function()
	local scripts = {}
	for _, obj in getInstanceList() do
		if typeof(obj) == "Instance" and (obj:IsA("ModuleScript") or obj:IsA("LocalScript")) then
			table.insert(scripts, obj)
		end
	end
	return scripts
end)

getgenv().getloadedmodules = newcclosure(function()
	local list = {}
	for i, v in getgc(false) do
		if typeof(v) == "function" then
			local env = getfenv(v)
			if typeof(env["script"]) == "Instance" and env["script"]:IsA("ModuleScript") then
				table_insert(list, env["script"])
			end
		end
	end
	return list
end)

getgenv().getsenv = newcclosure(function(scr)
	if typeof(scr) ~= "Instance" then
		error("Expected script. Got " .. typeof(script) .. " Instead.")
	end

	for _, obj in getgc(false) do
		if obj and typeof(obj) == "function" then
			local env = getfenv(obj)
			if env.script == scr then
				return getfenv(obj)
			end
		end
	end

	return {}
end)

getgenv().getrunningscripts = newcclosure(function()
	local scripts = {}

	for _, obj in getInstanceList() do
		if
			typeof(obj) == "Instance"
			and (obj:IsA("LocalScript") or (obj:IsA("Script") and obj.RunContext == "Client"))
			and obj.Enabled
		then
			table.insert(scripts, obj)
		end
	end

	return scripts
end)

local originalRequire = require
getgenv().require = function(module)
    if typeof(module) ~= "Instance" then error("Attempted to call require with invalid argument(s).") end
    if not module:IsA("ModuleScript") then error("Attempted to call require with invalid argument(s).") end
    local originalIdentity = getidentity()
    setidentity(2)
    local success, result = pcall(originalRequire, module)
    setidentity(originalIdentity)
    if not success then error(result) end
    return result
end

getgenv().GetObjects = function(assetId)
	local oldId = getidentity()
	setidentity(8)
    local obj = {}
    if (game:GetService("RunService"):IsClient()) then
	    obj = { insertservice_LoadLocalAsset(cloneref(game:GetService("InsertService")), assetId) }
    else
        obj = { insertservice_LoadAsset(cloneref(game:GetService("InsertService")), string_gsub(assetId, "rbxassetid://", "")) }
    end
	setIdentity_c(oldId)
	return obj
end
)"));
}
