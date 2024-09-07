//
// Created by Dottik on 7/9/2024.
//

#pragma once
#include <cstdint>

#include "lua.h"
#include "lualib.h"

#define RBXSTU_EXPORT __declspec(dllexport)
#define RBXSTU_MOD_REVISION 0

struct RbxStuInformation_t {
    /// @brief The version number RbxStu V2's mod loader is on. Useful for delimiting incompatibilities and other
    /// things.
    const int64_t llVersionNumber;
    /// @brief The number of mods loaded.
    const int64_t llLoadedModCount;
};

using RbxStuMod_Initialize = void(__fastcall *)();
using RbxStuMod_OnSchedulerInitialized = void(__fastcall *)(lua_State *L);
using RbxStuMod_CloseMod = void(__fastcall *)();

using RbxStuMod_Setup = void(__fastcall *)();

struct RbxStuModContext_t {
    const char *modName;
    const char *modDescription;
    int64_t modVersion;
    RbxStuMod_Initialize lpInitializationRoutine;
    RbxStuMod_OnSchedulerInitialized lpOnSchedulerInitializedRoutine;
    RbxStuMod_CloseMod lpClosingRoutine;
};

RBXSTU_EXPORT _Out_ RbxStuInformation_t GetRbxStuInformation();

RBXSTU_EXPORT _Out_ RbxStuModContext_t *
CreateRbxStuModContext(_In_ const char *modName, _In_ const char *modDescription, _In_ int64_t modVersion,
                       RbxStuMod_Initialize lpInitializationRoutine,
                       RbxStuMod_OnSchedulerInitialized lpOnSchedulerInitializedRoutine,
                       RbxStuMod_CloseMod lpClosingRoutine);

RBXSTU_EXPORT void RegisterLuauLibrary(_In_ RbxStuModContext_t *modContext, _In_ const char *libname,
                                       _In_ luaL_Reg *funcs);

RBXSTU_EXPORT void DestroyRbxStuModContext(_In_ RbxStuModContext_t *);
