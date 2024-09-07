//
// Created by Dottik on 7/9/2024.
//

#include "ModManager.hpp"

#include "Logger.hpp"

std::shared_ptr<ModManager> ModManager::pInstance;

std::shared_ptr<ModManager> ModManager::GetSingleton() {
    if (ModManager::pInstance == nullptr)
        ModManager::pInstance = std::make_shared<ModManager>();

    return ModManager::pInstance;
}
void ModManager::LoadMods() {
    const auto logger = Logger::GetSingleton();
    logger->PrintInformation(RbxStu::ModManager, "Building Mod List...");
    auto dllDir = Utilities::GetDllDir();
    auto path = std::filesystem::path(dllDir);

    path /= "Mods";
    if (std::filesystem::exists(path)) {
        for (const auto &entry: std::filesystem::directory_iterator(path)) {
            if (entry.exists()) {
                if (const auto name = entry.path().filename().string(); name.find(".dll") != std::string::npos) {
                    logger->PrintInformation(RbxStu::ModManager, std::format("Found loadable module -- '{}'!", name));
                    const auto hModule = LoadLibraryW(entry.path().c_str());
                    if (auto setup = GetProcAddress(hModule, "ModSetup"); setup == nullptr) {
                        FreeLibrary(hModule);
                        logger->PrintWarning(
                                RbxStu::ModManager,
                                "Failed to find procedure 'ModSetup' on the remote shared library. Library unloaded.");
                    } else {
                        logger->PrintInformation(RbxStu::ModManager, "Running setup!");
                        const auto oldSize = this->m_modList.size();
                        reinterpret_cast<RbxStuMod_Setup>(setup)();
                        if (oldSize == this->m_modList.size()) {
                            logger->PrintError(RbxStu::ModManager,
                                               "The mod has not registered itself. This may lead to invalid behaviour. "
                                               "Register the mod at the Setup phase.");

                            throw std::exception("ModLoader: Failure. Cannot load module if the module does not "
                                                 "initialize its Mod context!");
                        } else {
                            const auto modCtx = this->m_modList.back();
                            logger->PrintInformation(
                                    RbxStu::ModManager,
                                    std::format("Mod registered!\nMod Name:{}\nMod Description: {}\nMod Version: {:#x}",
                                                modCtx->modName, modCtx->modDescription, modCtx->modVersion));
                        }
                    }
                } else {
                }
            }
        }
        logger->PrintInformation(
                RbxStu::ModManager,
                std::format("Mod search complete! {} mods have been loaded into RbxStu!", this->m_modList.size()));
    } else {
        logger->PrintInformation(RbxStu::ModManager, "There is no mods folder! Creating stub folder...");
        std::filesystem::create_directory(path.string());
        logger->PrintInformation(
                RbxStu::ModManager,
                "Created! Mods can be now loaded if placed there and they match up to the RbxStu V2 mod spec!");

        logger->PrintInformation(RbxStu::ModManager, "No mods loaded.");
        return;
    }
}
RbxStuInformation_t ModManager::GetRbxStuInformation(RbxStuModContext_t *modContext) {
    const auto logger = Logger::GetSingleton();
    if (!Utilities::IsPointerValid(modContext)) {
        logger->PrintError(RbxStu::ModManager, "Attempted to retrieve RbxStu Information with an invalid mod context!");
        throw std::exception("You cannot retrieve RbxStu's Information with an invalid mod context. Did you free or "
                             "pass in an uninitialized pointer?");
    }
    logger->PrintInformation(RbxStu::ModManager, "RbxStu information obtained!");

    return {0, static_cast<int64_t>(this->m_modList.size())};
}

void ModManager::InitializeMods() {
    const auto logger = Logger::GetSingleton();
    for (const auto &mod: this->m_modList) {
        if (mod->lpInitializationRoutine) {
            logger->PrintInformation(RbxStu::ModManager,
                                     std::format("Executing Initialization routine for mod {}!", mod->modName));
            mod->lpInitializationRoutine();
        }
    }
}

void ModManager::OnSchedulerInitialized(lua_State *L) {
    const auto logger = Logger::GetSingleton();
    for (const auto &mod: this->m_modList) {
        if (mod->lpOnSchedulerInitializedRoutine) {
            logger->PrintInformation(RbxStu::ModManager,
                                     std::format("Executing OnSchedulerInitialized Routine for mod {}!", mod->modName));
            mod->lpOnSchedulerInitializedRoutine(L);

            if (!Utilities::IsPointerValid(L)) {
                logger->PrintInformation(
                        RbxStu::ModManager,
                        std::format("FATAL! lua_State was freed during a OnSchedulerInitialized callback! This has "
                                    "placed RbxStu V2 in an irrecoverable state! Faulty mod: '{}'",
                                    mod->modName));
                throw std::exception("lua state was freed by the OnSchedulerInitialized mod routine!");
            }
        }
    }
}

void ModManager::RegisterMod(RbxStuModContext_t *modContext) {
    const auto logger = Logger::GetSingleton();
    if (!Utilities::IsPointerValid(modContext)) {
        logger->PrintError(RbxStu::ModManager, "Attempted to register a mod with an invalid mod context!");
        throw std::exception("You cannot register a mod with an invalid context. Did you free or "
                             "pass in an uninitialized pointer?");
    }
    this->m_modList.push_back(modContext);
}
void ModManager::UnloadMod(RbxStuModContext_t *modContext) {
    const auto logger = Logger::GetSingleton();
    if (!Utilities::IsPointerValid(modContext)) {
        logger->PrintError(RbxStu::ModManager, "Attempted to unregister a mod with an invalid mod context!");
        throw std::exception("You cannot unregister a mod with an invalid context. Did you free or "
                             "pass in an uninitialized pointer?");
    }

    modContext->lpClosingRoutine();

    for (auto it = this->m_modList.begin(); it != this->m_modList.end();) {
        if (*it == modContext)
            it = this->m_modList.erase(it);
        else
            ++it;
    }
}
