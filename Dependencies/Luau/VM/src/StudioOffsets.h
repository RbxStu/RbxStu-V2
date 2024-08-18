//
// Created by Dottik on 3/4/2024.
//
#pragma once

#include <cstdint>
#include <memory>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>

struct lua_TValue;

class RbxStuOffsets final
{
    static std::shared_ptr<RbxStuOffsets> ptr;
    static std::shared_mutex __rbxstuoffsets__sharedmutex__;

public:
    std::map<std::string, void*> offsets;

    static std::shared_ptr<RbxStuOffsets> GetSingleton()
    {
        std::lock_guard lock{__rbxstuoffsets__sharedmutex__};
        if (ptr == nullptr)
            ptr = std::make_shared<RbxStuOffsets>();
        return ptr;
    }

    void* GetOffset(const std::string& str)
    {
        return this->offsets[str];
    }

    void SetOffset(const std::string& str, void* func)
    {
        this->offsets[str] = func;
    }
};

struct lua_State;

namespace RBX::Studio::FunctionTypes
{
using luau_execute = void(__fastcall*)(lua_State* L);
using pseudo2addr = lua_TValue*(__fastcall*)(lua_State* L, int32_t lua_index);
using rTask_defer = int(__fastcall*)(lua_State* L);
using luaE_newthread = lua_State*(__fastcall*)(lua_State* L);
using lua_newthread = lua_State*(__fastcall*)(lua_State* L);
using rFromLuaState = void(__fastcall*)(lua_State* LP, lua_State* L);
using freeBlock = void(__fastcall*)(lua_State* L, int32_t sizeClass, void* block);
using luaD_throw = void(__fastcall*)(lua_State* L, int32_t errcode);
using luaD_rawrununprotected = int32_t(__fastcall*)(lua_State* L, void (*PFunc)(lua_State* L, void* ud), void* ud);
using luaC_Step = size_t(__fastcall*)(lua_State* L, bool assist);
using fireproximityprompt = void(__fastcall*)(void* proximityPrompt);
using pushinstance = std::uintptr_t(__fastcall*)(lua_State* L, void* instance);
using luaV_gettable = void(__fastcall*)(lua_State* L, const void* t, const void* key, void* val);
using luaV_settable = void(__fastcall*)(lua_State* L, const void* t, const void* key, void* val);
}; // namespace RBX::Studio::FunctionTypes

/*
 *  How to get this to compile when updating Luau?
 *      - Modify lobject.cpp and lobject.h to use Studios' luaO_nilobject, same thing with ltable.cpp and ltable.h and luaH_dummynode, as well as
 * modifying lvm.cpp to use luau_execute. You must use luau_load when compiling code, for anyone using this to develop anything.
 */