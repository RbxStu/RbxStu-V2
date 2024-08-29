//
// Created by Pixeluted on 22/08/2024.
//

#include "Misc.hpp"

#include <HttpStatus.hpp>
#include <lz4.h>

#include "Communication.hpp"
#include "RobloxManager.hpp"
#include "Scheduler.hpp"
#include "cpr/api.h"

namespace RbxStu {
    namespace Misc {
        int identifyexecutor(lua_State *L) {
            lua_pushstring(L, "RbxStu");
            lua_pushstring(L, "V2");
            return 2;
        }

        int lz4compress(lua_State *L) {
            luaL_checktype(L, 1, LUA_TSTRING);
            const char *data = lua_tostring(L, 1);
            int iMaxCompressedSize = LZ4_compressBound(strlen(data));
            const auto pszCompressedBuffer = new char[iMaxCompressedSize];
            memset(pszCompressedBuffer, 0, iMaxCompressedSize);

            LZ4_compress(data, pszCompressedBuffer, strlen(data));
            lua_pushlstring(L, pszCompressedBuffer, iMaxCompressedSize);
            return 1;
        }

        int lz4decompress(lua_State *L) {
            luaL_checktype(L, 1, LUA_TSTRING);
            luaL_checktype(L, 2, LUA_TNUMBER);

            const char *data = lua_tostring(L, 1);
            const int data_size = lua_tointeger(L, 2);

            auto *pszUncompressedBuffer = new char[data_size];

            memset(pszUncompressedBuffer, 0, data_size);

            LZ4_uncompress(data, pszUncompressedBuffer, data_size);
            lua_pushlstring(L, pszUncompressedBuffer, data_size);
            return 1;
        }

        int messagebox(lua_State *L) {
            const auto text = luaL_checkstring(L, 1);
            const auto caption = luaL_checkstring(L, 2);
            const auto type = luaL_checkinteger(L, 3);
            Scheduler::GetSingleton()->ScheduleJob(
                    SchedulerJob(L, [text, caption, type](lua_State *L,
                                                          std::shared_future<std::function<int(lua_State *)>> *future) {
                        const int lMessageboxReturn = MessageBoxA(nullptr, text, caption, type);

                        *future = std::async(std::launch::async,
                                             [lMessageboxReturn]() -> std::function<int(lua_State *)> {
                                                 return [lMessageboxReturn](lua_State *L) -> int {
                                                     lua_pushinteger(L, lMessageboxReturn);
                                                     return 1;
                                                 };
                                             });
                    }));

            L->ci->flags |= 1;
            return lua_yield(L, 1);
        }

        int setclipboard(lua_State *L) {
            luaL_checkstring(L, 1);
            const char *text = lua_tostring(L, 1);

            if (!OpenClipboard(nullptr))
                luaL_error(L, "Failed to open clipboard");


            if (!EmptyClipboard()) {
                CloseClipboard();
                luaL_error(L, "Failed to empty clipboard");
            }

            const size_t textLength = strlen(text) + 1;
            const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, textLength);
            if (!hMem) {
                CloseClipboard();
                luaL_error(L, "Failed to allocate memory in global space");
            }

            const auto pMem = static_cast<char *>(GlobalLock(hMem));
            if (!pMem) {
                GlobalFree(hMem);
                CloseClipboard();
                luaL_error(L, "Failed to lock global memory");
            }

            memcpy(pMem, text, textLength);

            if (GlobalUnlock(hMem) != 0 && GetLastError() != NO_ERROR) {
                GlobalFree(hMem);
                CloseClipboard();
                luaL_error(L, "Failed to unlock global memory");
            }

            if (!SetClipboardData(CF_TEXT, hMem)) {
                GlobalFree(hMem);
                CloseClipboard();
                luaL_error(L, "Failed to set clipboard data");
            }

            if (!CloseClipboard())
                luaL_error(L, "Failed to close clipboard");

            return 0;
        }

        int getclipboard(lua_State *L) {
            if (!Communication::GetSingleton()->IsUnsafeMode()) {
                Logger::GetSingleton()->PrintInformation(
                        RbxStu::Anonymous,
                        "getclipboard call has returned a stub value, as getclipboard is considered an "
                        "unsafe function, and will only be available when Unsafe mode is enabled.");
                lua_pushstring(L, "");
                return 1;
            }

            if (!OpenClipboard(nullptr))
                luaL_error(L, "Failed to open clipboard");

            HANDLE hData = GetClipboardData(CF_TEXT);
            if (!hData) {
                CloseClipboard();
                luaL_error(L, "Failed to get clipboard data");
            }

            const char *clipboardText = static_cast<const char *>(GlobalLock(hData));
            if (!clipboardText) {
                CloseClipboard();
                luaL_error(L, "Failed to lock clipboard data");
            }

            lua_pushstring(L, clipboardText);

            if (GlobalUnlock(hData) == 0 && GetLastError() != NO_ERROR) {
                CloseClipboard();
                luaL_error(L, "Failed to unlock clipboard data");
            }

            if (!CloseClipboard())
                luaL_error(L, "Failed to close clipboard");

            return 1;
        }

        int emptyclipboard(lua_State *L) {
            if (!OpenClipboard(nullptr))
                luaL_error(L, "Failed to open clipboard");


            if (!EmptyClipboard()) {
                CloseClipboard();
                luaL_error(L, "Failed to empty clipboard");
            }


            if (!CloseClipboard())
                luaL_error(L, "Failed to close clipboard");

            return 0;
        }

        int httpget(lua_State *L) {
            const std::string url = luaL_checkstring(L, 1);

            if (url.find("http://") == std::string::npos && url.find("https://") == std::string::npos)
                luaL_argerror(L, 1, "Invalid protocol (expected 'http://' or 'https://')");

            const auto scheduler = Scheduler::GetSingleton();

            scheduler->ScheduleJob(SchedulerJob(
                    L, [url](lua_State *L, std::shared_future<std::function<int(lua_State *)>> *callbackToExecute) {
                        const auto response = cpr::Get(cpr::Url{url}, cpr::Header{{"User-Agent", "Roblox/WinInet"}});

                        auto output = std::string("");

                        if (HttpStatus::IsError(response.status_code)) {
                            output = std::format("HttpGet failed\nResponse {} - {}. {}",
                                                 std::to_string(response.status_code),
                                                 HttpStatus::ReasonPhrase(response.status_code),
                                                 std::string(response.error.message));
                        } else {
                            output = response.text;
                        }

                        *callbackToExecute =
                                std::async(std::launch::async, [output]() -> std::function<int(lua_State *)> {
                                    return [output](lua_State *L) -> int {
                                        lua_pushlstring(L, output.c_str(), output.size());
                                        return 1;
                                    };
                                });
                    }));

            L->ci->flags |= 1;
            return lua_yield(L, 1);
        }

        int getfpscap(lua_State *L) {
            const auto possiblePointerToTargetFps =
                    RobloxManager::GetSingleton()->GetFastVariable("TaskSchedulerTargetFps");
            if (!possiblePointerToTargetFps.has_value())
                luaL_error(L,
                           "cannot getfpscap: The Fast Variable of type Int 'TaskSchedulerTargetFps' is unavailable!");
            lua_pushinteger(L, *static_cast<int32_t *>(possiblePointerToTargetFps.value()));

            return 1;
        }

        int setfpscap(lua_State *L) {
            luaL_checkinteger(L, 1);

            auto num = luaL_optinteger(L, 1, 60);

            if (num <= 0)
                num = 1000;

            const auto possiblePointerToTargetFps =
                    RobloxManager::GetSingleton()->GetFastVariable("TaskSchedulerTargetFps");
            if (!possiblePointerToTargetFps.has_value())
                luaL_error(L,
                           "cannot setfpscap: The Fast Variable of type Int 'TaskSchedulerTargetFps' is unavailable!");

            *static_cast<int32_t *>(possiblePointerToTargetFps.value()) = num;
            return 0;
        }

        const std::array<std::string, 5> validMethods = {
            "get", "post", "put", "patch", "delete"
        };

        bool isValidHttpMethod(const std::string& method) {
            return std::find(validMethods.begin(), validMethods.end(), method) != validMethods.end();
        }

        int request(lua_State *L) {
            luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
            lua_getfield(L, 1, "Url");
            if (lua_isnil(L, -1))
                luaL_argerrorL(L, 1, "Options must have a Url field");
            if (!lua_isstring(L, -1))
                luaL_argerrorL(L, 1, "Url must be a string");
            lua_pop(L, 1);

            lua_getfield(L, 1, "Method");
            if (lua_isnil(L, -1))
                luaL_argerrorL(L, 1, "Options must have a Method field");
            if (!lua_isstring(L, -1))
                luaL_argerrorL(L, 1, "Method must be a string");
            lua_pop(L, 1);

            lua_getfield(L, 1, "Url");
            std::string Url = _strdup(lua_tostring(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, 1, "Method");
            std::string Method = Utilities::ToLower(_strdup(lua_tostring(L, -1)));
            lua_pop(L, 1);
            if (!isValidHttpMethod(Method))
                luaL_argerrorL(L, 1, "Method must be one of these 'GET', 'POST', 'PUT', 'PATCH', 'DELETE'");

            auto Headers = std::list<std::pair<std::string, std::string>>();
            Headers.emplace_back("User-Agent", "Roblox/WinInet");

            lua_getfield(L, 1, "Headers");
            if (!lua_isnil(L, -1)) {
                if (!lua_istable(L, -1))
                    luaL_argerrorL(L, 1, "Headers must a be table");

                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    if (!lua_isstring(L, -2))
                        luaL_argerrorL(L, 1, "Key of a header must be a string");

                    if (!lua_isstring(L, -1))
                        luaL_argerrorL(L, 1, "Value of a header must be a string");

                    std::string headerName = _strdup(lua_tostring(L, -2));
                    std::string headerValue = _strdup(lua_tostring(L, -1));
                    Headers.emplace_back(headerName, headerValue);

                    lua_pop(L, 1);
                }

                lua_pop(L, 1);
            }

            cpr::Cookies cookies;
            lua_getfield(L, 1, "Cookies");
            if (!lua_isnil(L, -1)) {
                if (!lua_istable(L, -1))
                    luaL_argerrorL(L, 1, "Cookies must be a table");

                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    if (!lua_isstring(L, -2))
                        luaL_argerrorL(L, 1, "Key of a cookie must be a string");

                    if (!lua_isstring(L, -1))
                        luaL_argerrorL(L, 1, "Value of a cookie must be a string");

                    std::string cookieName = _strdup(lua_tostring(L, -2));
                    std::string cookieValue = _strdup(lua_tostring(L, -1));
                    cookies.emplace_back(cpr::Cookie{cookieName, cookieValue});

                    lua_pop(L, 1);
                }

                lua_pop(L, 1);
            }

            const auto scheduler = Scheduler::GetSingleton();

            scheduler->ScheduleJob(SchedulerJob(
                    L, [Url, Method, Headers, cookies](lua_State *L, std::shared_future<std::function<int(lua_State *)>> *callbackToExecute) {
                        cpr::Session requestSession;
                        requestSession.SetUrl(Url);

                        std::map<std::string, std::string, cpr::CaseInsensitiveCompare> headerMap;
                        for (const auto& header : Headers) {
                            headerMap[header.first] = header.second;
                        }

                        requestSession.SetOption(cpr::Header(headerMap));
                        requestSession.SetOption(cookies);

                        if (Method == "get") {
                            requestSession.PrepareGet();
                        } else if (Method == "post") {
                            requestSession.PreparePost();
                        } else if (Method == "put") {
                            requestSession.PreparePut();
                        } else if (Method == "patch") {
                            requestSession.PreparePatch();
                        } else if (Method == "delete") {
                            requestSession.PrepareDelete();
                        }

                        CURLcode curl_result = curl_easy_perform(requestSession.GetCurlHolder()->handle);
                        cpr::Response response = requestSession.Complete(curl_result);

                        *callbackToExecute =
                                std::async(std::launch::async, [response]() -> std::function<int(lua_State *)> {
                                    return [response](lua_State *L) -> int {
                                        lua_newtable(L);
                                        lua_pushboolean(L, !HttpStatus::IsError(response.status_code) && response.status_code != 0);
                                        lua_setfield(L, -2, "Success");

                                        lua_pushnumber(L, response.status_code);
                                        lua_setfield(L, -2, "StatusCode");

                                        lua_pushstring(L, response.text.c_str());
                                        lua_setfield(L, -2, "Body");

                                        return 1;
                                    };
                                });
                    }));

            return lua_yield(L, 1);
        }
    } // namespace Misc
} // namespace RbxStu

std::string Misc::GetLibraryName() { return "misc"; }
luaL_Reg *Misc::GetLibraryFunctions() {
    auto reg = new luaL_Reg[]{{"identifyexecutor", RbxStu::Misc::identifyexecutor},
                              {"getexecutorname", RbxStu::Misc::identifyexecutor},

                              {"lz4compress", RbxStu::Misc::lz4compress},
                              {"lz4decompress", RbxStu::Misc::lz4decompress},

                              {"messagebox", RbxStu::Misc::messagebox},
                              {"getfpscap", RbxStu::Misc::getfpscap},
                              {"setfpscap", RbxStu::Misc::setfpscap},

                              {"setclipboard", RbxStu::Misc::setclipboard},
                              {"toclipboard", RbxStu::Misc::setclipboard},
                              {"getclipboard", RbxStu::Misc::getclipboard},
                              {"emptyclipboard", RbxStu::Misc::emptyclipboard},

                              {"httpget", RbxStu::Misc::httpget},
                              {"request", RbxStu::Misc::request},

                              {nullptr, nullptr}};

    return reg;
}
