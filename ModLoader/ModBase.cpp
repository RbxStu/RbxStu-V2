//
// Created by Dottik on 7/9/2024.
//

#pragma once
#include "ModLoader/ModBase.h"

#include "Environment/EnvironmentManager.hpp"
#include "ModLoader/ModManager.hpp"

RbxStuInformation_t GetRbxStuInformation(RbxStuModContext_t *modContext) {
    return ModManager::GetSingleton()->GetRbxStuInformation(modContext);
}
RbxStuModContext_t *CreateRbxStuModContext(const char *modName, const char *modDescription, const int64_t modVersion,
                                           RbxStuMod_Initialize lpInitializationRoutine,
                                           RbxStuMod_OnSchedulerInitialized lpOnSchedulerInitializedRoutine,
                                           RbxStuMod_CloseMod lpClosingRoutine) {
    auto *modContext = new RbxStuModContext_t{};
    const auto cModName = new char[strlen(modName) + 1];
    memcpy(cModName, modName, strlen(modName));
    cModName[strlen(modName)] = 0;
    modContext->modName = cModName;

    const auto cModDescription = new char[strlen(modDescription) + 1];
    memcpy(cModDescription, modDescription, strlen(modDescription));
    cModDescription[strlen(modDescription)] = 0;
    modContext->modDescription = cModDescription;

    modContext->modVersion = modVersion;
    modContext->lpInitializationRoutine = lpInitializationRoutine;
    modContext->lpOnSchedulerInitializedRoutine = lpOnSchedulerInitializedRoutine;
    modContext->lpClosingRoutine = lpClosingRoutine;

    ModManager::GetSingleton()->RegisterMod(modContext);

    return modContext;
}

void DestroyRbxStuModContext(_In_ RbxStuModContext_t *modCtx) {
    ModManager::GetSingleton()->UnloadMod(modCtx);
    delete modCtx->modDescription;
    delete modCtx->modName;
    delete modCtx;
}

void RegisterLuauLibrary(RbxStuModContext_t *modContext, const char *libname, luaL_Reg *funcs) {
    const auto logger = Logger::GetSingleton();
    if (!Utilities::IsPointerValid(modContext)) {
        logger->PrintError(RbxStu::ModManager, "Attempted to register a luau library with an invalid mod context!");
        throw std::exception("You cannot register a luau library with an invalid context. Did you free or "
                             "pass in an uninitialized pointer?");
    }

    EnvironmentManager::GetSingleton()->DeclareLibrary(libname, funcs);
}
