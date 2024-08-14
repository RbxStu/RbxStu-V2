//
// Created by Dottik on 13/8/2024.
//

#include "LuauManager.hpp"
#include <shared_mutex>
#include "RobloxManager.hpp"
#include "Scanner.hpp"
#include "lobject.h"
#include "lualib.h"


namespace RbxStu {
#define _MakeSignature_FromIDA(signatureName, idaSignature)                                                            \
    const static Signature signatureName = SignatureByte::GetSignatureFromIDAString(idaSignature)

    namespace LuauFunctionDefinitions {
        using luaH_new = void *(__fastcall *) (void *L, int32_t narray, int32_t nhash);
    }

    namespace LuauSignatures {
        _MakeSignature_FromIDA(_luaD_throw, "48 83 EC ? 44 8B C2 48 8B D1 48 8D 4C 24 ? E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 "
                                            "8D 4C 24 ? E8 ? ? ? ? CC CC CC");
        _MakeSignature_FromIDA(_luau_execute, "80 79 06 00 0F 85 ? ? ? ? E9 ? ? ? ? CC");
        _MakeSignature_FromIDA(_pseudo2addr, "41 B9 EE D8 FF FF 4C 8B C1 41 3B D1 0F 84 88 00 00 00");
        _MakeSignature_FromIDA(_luaE_newthread,
                               "48 89 5C 24 ? 57 48 83 EC ? 44 0F B6 41 ? BA ? ? ? ? 48 8B F9 E8 ? ? ? "
                               "? 48 8B 57 ? 48 8B D8 44 0F B6 42 ? C6 00 ? 41 80 E0 ? 44 88 40 ?");
        _MakeSignature_FromIDA(_luaC_step, "48 8B 59 ? B8 ? ? ? ? 0F B6 F2 0F 29 74 24 ? 4C 8B F1 44 8B 43 ?");

        _MakeSignature_FromIDA(
                _luaD_rawrunprotected,
                "48 89 4C 24 ? 48 83 EC ? 48 8B C2 49 8B D0 FF D0 33 C0 EB 04 8B 44 24 48 48 83 C4 ? C3");

        _MakeSignature_FromIDA(_luaH_new,
                               "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B F0 8B "
                               "EA 44 0F B6 41 ? BA ? ? ? ? 48 8B F9 E8 ? ? ? ? 4C 8B 4F ? 48 8B D8 45 0F B6 "
                               "51 ? C6 00 ? 41 80 E2 ? 44 88 50 ? 44 0F B6 47 ? 44 88 40 ?");

        static std::map<std::string, Signature> s_luauSignatureMap = {
                {"luaD_throw", _luaD_throw},   {"luau_execute", _luau_execute},
                {"pseudo2addr", _pseudo2addr}, {"luaE_newthread", _luaE_newthread},
                {"luaC_step", _luaC_step},     {"luaD_rawrunprotected", _luaD_rawrunprotected},
                {"luaH_new", _luaH_new}};
        // TODO: Assess whether freeblock is required once again to be hooked due to stability issues.
    } // namespace LuauSignatures

#undef _MakeSignature_FromIDA
} // namespace RbxStu

static std::shared_mutex __luaumanager__singletonmutex;
static std::shared_ptr<LuauManager> __luaumanager__instance;

void LuauManager::Initialize() {
    const auto logger = Logger::GetSingleton();
    if (this->m_bIsInitialized) {
        logger->PrintWarning(RbxStu::LuauManager, "This instance is already initialized!");
        return;
    }
    const auto robloxManager = RobloxManager::GetSingleton();
    const auto scanner = Scanner::GetSingleton();

    logger->PrintInformation(RbxStu::LuauManager, "Initializing Luau Manager [1/4]");

    logger->PrintInformation(RbxStu::LuauManager, "Scanning functions (simple)... [1/4]");
    for (const auto &[fName, fSignature]: std::map<std::string, Signature>{}) {
        const auto results = scanner->Scan(fSignature);

        if (results.empty()) {
            logger->PrintWarning(RbxStu::LuauManager, std::format("Failed to find function '{}'!", fName));
        } else {
            if (results.size() > 1) {
                logger->PrintWarning(
                        RbxStu::LuauManager,
                        "More than one candidate has matched the signature. This is generally not something "
                        "problematic, but it may mean the signature has too many wildcards that makes it not "
                        "unique. "
                        "The first result will be chosen.");
            }
            this->m_mapLuauFunctions[fName] =
                    *results.data(); // Grab first result, it doesn't really matter to be honest.
        }
    }

    logger->PrintInformation(RbxStu::LuauManager, "Overwriting .data pointers for RVM [2/4]");
    RBX::Studio::Offsets::fireproximityprompt =
            reinterpret_cast<std::uintptr_t>(robloxManager->GetRobloxFunction("RBX::ProximityPrompt::onTriggered"));

#define MapFunction(funcName, mappedName)                                                                              \
    RBX::Studio::Offsets::funcName = reinterpret_cast<std::uintptr_t>(this->m_mapLuauFunctions[mappedName]);           \
    RBX::Studio::Functions::funcName =                                                                                 \
            reinterpret_cast<RBX::Studio::FunctionTypes::funcName>(RBX::Studio::Offsets::funcName)
    // MapFunction(luau_execute, "luau_execute");
    // MapFunction(luaD_throw, "luaD_throw");
    // MapFunction(luaE_newthread, "luaE_newthread");
    // MapFunction(luaC_Step, "luaC_step");
    // MapFunction(luaD_rawrununprotected, "luaD_rawrununprotected");
#undef MapFunction

    logger->PrintInformation(RbxStu::LuauManager, "Resolving data pointers to luaH_dummyNode and luaO_nilObject [3/4]");

    //  To obtain these data pointers, we must first obtain two key functions, pseudo2addr and luaH_new
    //  Then we create a lua_State ourselves and we make calls that make them return to either the stack or to a data
    //  structure the pointer to them. This is by far the cleanest and easiest method, and avoids sins like crawling
    //  assembly and doing binary tomfoolery.

    lua_State *luaState = luaL_newstate();
}
/*

    {
        RBX::Studio::Offsets::_luaO_nilobject =
                reinterpret_cast<std::uintptr_t>(reinterpret_cast<RBX::Studio::FunctionTypes::pseudo2addr>(
                        this->m_mapLuauFunctions["pseudo2addr"])(luaState, 1));
        logger->PrintInformation(RbxStu::LuauManager, std::format("Resolved luaO_nilObject to pointer {}",
                                                                  RBX::Studio::Offsets::_luaO_nilobject));

        auto luaH_new = reinterpret_cast<void *(__fastcall *) (void *L, int32_t narray, int32_t nhash)>(
                this->m_mapLuauFunctions["luaH_new"]);

        if (luaH_new == nullptr) {
            logger->PrintError(RbxStu::LuauManager, "Failed to obtain luaH_new, cannot resolve luaH_dummyNode!");

            throw std::exception(
                    "Failed to resolve required function using signatures, cowardly refusing to start execution");
        }

        auto table = luaH_new(luaState, 0, 0);
        RBX::Studio::Offsets::luaH_dummynode = reinterpret_cast<std::uintptr_t>(static_cast<Table *>(table)->node);
        logger->PrintInformation(RbxStu::LuauManager, std::format("Resolved luaH_dummyNode to pointer {}",
                                                                  RBX::Studio::Offsets::luaH_dummynode));
    }

    logger->PrintInformation(RbxStu::LuauManager, "Cleaning up lua_State used to obtain values... ");
    lua_close(luaState);
    logger->PrintInformation(RbxStu::LuauManager, "All cleaned up!");

    logger->PrintInformation(RbxStu::LuauManager, "Hooking functions... [3/4]");
    // No hooks required, YET!

    logger->PrintInformation(RbxStu::LuauManager, "Initialization completed [4/4]");
    this->m_bIsInitialized = true;
}

std::shared_ptr<LuauManager> LuauManager::GetSingleton() {
    std::lock_guard lock{__luaumanager__singletonmutex};
    if (__luaumanager__instance == nullptr)
        __luaumanager__instance = std::make_shared<LuauManager>();

    if (!__luaumanager__instance->m_bIsInitialized) {
        //     // __luaumanager__instance->Initialize();
    }

    return __luaumanager__instance;
}
*/
