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
    if (EnvironmentManager::pInstance == nullptr) {
        EnvironmentManager::pInstance = std::make_shared<EnvironmentManager>();
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Debug{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Globals{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Filesystem{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Closures{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Metatable{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Cache{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Console{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Script{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Misc{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Instance{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Input{});
        EnvironmentManager::pInstance->m_vLibraryList.push_back(new Http{});
    }

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
                                                   "scripteditorservice",
                                                   "avatareditorservice",
                                                   "webviewservice"};

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
        "startsessionwithpath", // StartSessionWithPathAsync
        "banasync", "makerequest", "setplaceid", "setuniverseid", "reportingoogleanalytics", "shutdown"};

static std::map<std::string, std::vector<std::string>> specificBlockage = {
        {std::string{"OpenCloudService"}, std::vector<std::string>{"RegisterOpenCloud"}},
        {std::string{"HttpService"}, std::vector<std::string>{"SetHttpEnabled"}},
        {std::string{"BrowserService"},
         std::vector<std::string>{
                 "CopyAuthCookieFromBrowserToEngine",
                 "EmitHybridEvent",
                 "ExecuteJavaScript",
                 "OpenBrowserWindow",
                 "OpenNativeOverlay",
                 "OpenWeChatAuthWindow",
                 "ReturnToJavaScript",
                 "SendCommand",
         }},
        {std::string{"DataModel"}, std::vector<std::string>{"Load"}},
        {std::string{"HttpRbxApiService"},
         std::vector<std::string>{
                 "PostAsync",
                 "PostAsyncFullUrl",
                 "GetAsync",
                 "GetAsyncFullUrl",
                 "RequestAsync",
                 "RequestLimitedAsync",

         }},
        {std::string{"MessageBusService"},
         std::vector<std::string>{
                 "Call",
                 "GetLast",
                 "GetMessageId",
                 "GetProtocolRequestMessageId",
                 "GetProtocolResponseMessageId",
                 "MakeRequest",
                 "Publish",
                 "PublishProtocolMethodRequest",
                 "PublishProtocolMethodResponse",
                 "SetRequestHandler",
                 "Subscribe",
                 "SubscribeToProtocolMethodRequest",
                 "SubscribeToProtocolMethodResponse",
         }},
        {std::string{"MarketplaceService"},
         std::vector<std::string>{
                 "GetRobuxBalance",
                 "GetUserSubscriptionDetailsInternalAsync",
                 "PerformBulkPurchase",
                 "PerformPurchase",
                 "PerformPurchaseV2",
                 "PerformSubscriptionPuchase",
                 "PerformSubscriptionPuchaseV2",
                 "PrepareCollectiblesPurchase",
                 "PlayerOwnsAsset",
                 "PlayerOwnsBundle",
                 "PromptBulkPurchase",
                 "PromptBundlePurchase",
                 "PromptCancelSubscription",
                 "PromptCollectiblesPurchase",
                 "PromptGamePassPurchase",
                 "PromptNativePurchaseWithLocalPlayer",
                 "PromptPremiumPurchase",
                 "PromptRobloxPurchase",
                 "PromptThirdPartyPurchase",
                 "ReportAssetSale",
         }},
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
void EnvironmentManager::DeclareLibrary(const char *libname, luaL_Reg *functionList) {
    const auto nLib = new FlexibleLibrary{libname, functionList};
    EnvironmentManager::GetSingleton()->m_vLibraryList.push_back(nLib);
}

void EnvironmentManager::PushEnvironment(_In_ lua_State *L) {
    const auto logger = Logger::GetSingleton();

    // Don't replace renv globals.
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, "_G");
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, "shared");
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, "_ENV");

    for (const std::vector<Library *> libList = this->m_vLibraryList; const auto &lib: libList) {
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

        // We cannot clear lib anymore, as the object will now prevail.
        // delete lib;
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
            if (lua_gettop(L) < 2 || lua_type(L, 1) != lua_Type::LUA_TUSERDATA ||
                lua_type(L, 2) != lua_Type::LUA_TSTRING || !Security::GetSingleton()->IsOurThread(L))
                return __index_game_original(L);

            try {
                auto index = luaL_optstring(L, 2, nullptr);

                if (index == nullptr)
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

                lua_getmetatable(L, 1);
                lua_getfield(L, -1, "__type");

                if (const auto s = lua_tostring(L, -1);
                    strcmp(s, "Instance") == 0 && Security::GetSingleton()->IsOurThread(L)) {
                    lua_pop(L, 3);
                    lua_pushstring(L, "ClassName");
                    __index_game_original(L);
                    const auto instanceClassName = Utilities::ToLower(lua_tostring(L, -1));
                    lua_pop(L, 2);
                    lua_pushstring(L, index);

                    const auto indexAsString = std::string(index);
                    for (const auto &[bannedName, sound]: specificBlockage) {
                        if (Utilities::ToLower(bannedName).find(instanceClassName) != std::string::npos) {
                            for (const auto &func: sound) {
                                if (indexAsString.find(func) != std::string::npos &&
                                    strstr(indexAsString.c_str(), func.c_str()) == indexAsString.c_str()) {
                                    goto banned__index;
                                }
                                if (func == "BLOCK_ALL" && strcmp(loweredIndex.c_str(), "classname") != 0 &&
                                    strcmp(loweredIndex.c_str(), "name") != 0)
                                    goto banned__index; // Block all regardless.
                            }
                        }
                    }
                } else {
                    lua_pop(L, 2);
                }

                if (index == nullptr)
                    return __index_game_original(L);

                if (!Communication::GetSingleton()->CanAccessScriptSource() && strcmp(index, "Source") == 0) {
                    Utilities::checkInstance(L, 1, "LuaSourceContainer");
                    lua_pushstring(L, "");
                    return 1;
                }

                if (loweredIndex.find("getservice") != std::string::npos ||
                    loweredIndex.find("findservice") != std::string::npos ||
                    strstr(index, "service") !=
                            nullptr) { // manages getservice, GetService, FindService and service all at once.
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
                                                RbxStu::Anonymous,
                                                std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                            "BLACKLISTED SERVICE! SERVICE ACCESSED: {}",
                                                            serviceName));

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
                                try {
                                    lua_pcall(L, 2, 1, 0);
                                } catch (const std::exception &ex) {
                                    Logger::GetSingleton()->PrintWarning(
                                            RbxStu::Anonymous,
                                            std::format(
                                                    "RBX::CRASH! AN IRRECOVERABLE ERROR HAS OCCURRED THAT SHOULD NOT "
                                                    "HAVE OCCURRED! REDIRECTING CALL!"));
                                    lua_pushstring(L, "fatal error while trying to call DataModel.GetService()");
                                    lua_error(L);
                                    return 1;
                                }
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
                    Logger::GetSingleton()->PrintWarning(
                            RbxStu::Anonymous, std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                           "BLACKLISTED FUNCTION/SERVICE! FUNCTION ACCESSED: {}",
                                                           index));
                    if (Communication::GetSingleton()->IsUnsafeMode())
                        return __index_game_original(L);

                    luaL_errorL(L, "This service/function has been blocked for security");
                }
            } catch (const std::exception &ex) {
                // Logger::GetSingleton()->PrintWarning(
                //         RbxStu::Anonymous, std::format("FATAL ERROR CAUGHT ON DATAMODEL::__INDEX -> {}", ex.what()));
            } catch (...) {
                // Logger::GetSingleton()->PrintWarning(RbxStu::Anonymous,
                //                                      "FATAL ERROR CAUGHT ON DATAMODEL::__INDEX -> UNKNOWN");
            }

            if (lua_gettop(L) > 2 &&
                lua_type(L, -1) == lua_Type::LUA_TSTRING) // The lua stack should only have self and the index.
                lua_error(L);
            return __index_game_original(L);
        };
        lua_pop(L, 1);
        lua_rawgetfield(L, -1, "__namecall");

        auto __namecall = lua_toclosure(L, -1);
        __namecall_game_original = __namecall->c.f;
        __namecall->c.f = [](lua_State *L) -> int {
            auto lastStackElement = luaA_toobject(L, -1);

            try {
                if (!Security::GetSingleton()->IsOurThread(L) || lua_gettop(L) == 0 ||
                    lua_type(L, 1) != lua_Type::LUA_TUSERDATA)
                    return __namecall_game_original(L);

                auto namecall = lua_namecallatom(L, nullptr);

                if (namecall == nullptr)
                    return __namecall_game_original(L);

                const auto stack = lua_gettop(L);

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

                lua_getmetatable(L, 1);
                lua_getfield(L, -1, "__type");

                if (const auto s = lua_tostring(L, -1); strcmp(s, "Instance") == 0) {
                    lua_pushvalue(L, 1);
                    lua_getfield(L, -1, "ClassName");
                    const auto instanceClassName = Utilities::ToLower(lua_tostring(L, -1));
                    lua_settop(L, stack);

                    const auto namecallAsString = std::string(namecall);

                    for (const auto &[bannedName, sound]: specificBlockage) {
                        if (Utilities::ToLower(bannedName).find(instanceClassName) != std::string::npos) {
                            for (const auto &func: sound) {
                                if (namecallAsString.find(func) != std::string::npos) {
                                    goto banned__namecall;
                                }
                                if (func == "BLOCK_ALL")
                                    goto banned__namecall; // Block all regardless.
                            }
                        }
                    }
                } else {
                    lua_settop(L, stack);
                }

                if (false) {
                banned__namecall:
                    Logger::GetSingleton()->PrintWarning(
                            RbxStu::Anonymous, std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                           "BLACKLISTED FUNCTION/SERVICE! FUNCTION ACCESSED: {}",
                                                           namecall));
                    if (Communication::GetSingleton()->IsUnsafeMode())
                        return __namecall_game_original(L);

                    luaL_errorL(L, "This service/function has been blocked for security");
                }

                if (loweredNamecall.find("getservice") != std::string::npos ||
                    loweredNamecall.find("findservice") != std::string::npos ||
                    loweredNamecall.find("service") != std::string::npos) {
                    // getservice / findservice / service
                    if (lua_type(L, 2) != lua_Type::LUA_TSTRING)
                        return __namecall_game_original(L);

                    auto serviceName = luaL_checkstring(L, 2);
                    const auto svcName = Utilities::ToLower(serviceName);
                    for (const auto &func: blockedServices) {
                        if (svcName.find(func) != std::string::npos) {
                            Logger::GetSingleton()->PrintWarning(
                                    RbxStu::Anonymous, std::format("WARNING! AN ELEVATED THREAD HAS ACCESSED A "
                                                                   "BLACKLISTED SERVICE! SERVICE ACCESSED: {}",
                                                                   serviceName));

                            if (Communication::GetSingleton()->IsUnsafeMode())
                                break;

                            luaL_errorL(L, "This service has been blocked for security");
                        }
                    }
                    return __namecall_game_original(L);
                }

                if (loweredNamecall.find("httpget") != std::string::npos ||
                    loweredNamecall.find("httpgetasync") != std::string::npos) {
                    if (lua_type(L, 2) != lua_Type::LUA_TSTRING)
                        return __namecall_game_original(L);
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
            } catch (const std::exception &ex) {
                // Logger::GetSingleton()->PrintWarning(
                //         RbxStu::Anonymous, std::format("FATAL ERROR CAUGHT ON DATAMODEL::__NAMECALL -> {}",
                //         ex.what()));
            } catch (...) {
                // Logger::GetSingleton()->PrintWarning(RbxStu::Anonymous,
                //                                      "FATAL ERROR CAUGHT ON DATAMODEL::__NAMECALL -> UNKNOWN");
            }

            if (L->top - 1 != lastStackElement && lua_type(L, -1) == lua_Type::LUA_TSTRING) {
                lua_error(L);
            }

            return __namecall_game_original(L);
        };

        lua_settop(L, 0);
    } catch (const std::exception &ex) {
        logger->PrintError(RbxStu::EnvironmentManager,
                           std::format("Failed to initialize RbxStu Environment. Error from Lua: {}", ex.what()));
    }


    Scheduler::GetSingleton()->ScheduleJob(SchedulerJob(
            R"(
local insertservice_LoadLocalAsset = clonefunction(cloneref(game.GetService(game, "InsertService")).LoadLocalAsset)
local insertservice_LoadAsset = clonefunction(cloneref(game.GetService(game, "InsertService")).LoadAsset)
local table_insert = clonefunction(table.insert)
local table_find = clonefunction(table.find)
local setuntouched = clonefunction(setuntouched)
local isuntouched = clonefunction(isuntouched)
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
			if typeof(env["script"]) == "Instance" and env["script"]:IsA("ModuleScript") and not table_find(list, env["script"]) then
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

	return nil
end)

getgenv().getrunningscripts = newcclosure(function()
	local scripts = {}

	for _, obj in getInstanceList() do
		if
			typeof(obj) == "Instance"
			and (obj:IsA("LocalScript") or (obj:IsA("Script") and obj.RunContext == "Client"))
			and obj.Enabled
            and getsenv(obj) ~= nil
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

getgenv().scheduler_ = nil
)"));
}
