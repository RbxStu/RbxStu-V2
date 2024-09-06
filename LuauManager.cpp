//
// Created by Dottik on 13/8/2024.
//

#include "LuauManager.hpp"

#include <MinHook.h>
#include <StudioOffsets.h>
#include <shared_mutex>
#include "RobloxManager.hpp"
#include "Scanner.hpp"
#include "Scheduler.hpp"
#include "Security.hpp"
#include "lobject.h"
#include "lualib.h"


namespace RbxStu {
#define _MakeSignature_FromIDA(signatureName, idaSignature)                                                            \
    const static Signature signatureName = SignatureByte::GetSignatureFromIDAString(idaSignature)

    namespace LuauFunctionDefinitions {
        using luaH_new = void *(__fastcall *) (void *L, int32_t narray, int32_t nhash);
        using freeblock = void(__fastcall *)(lua_State *L, int32_t sizeClass, void *block);
        using lua_pushvalue = void(__fastcall *)(lua_State *L, int idx);
        using luaE_newthread = lua_State *(__fastcall *) (lua_State *L);
    } // namespace LuauFunctionDefinitions

    namespace LuauSignatures {
        _MakeSignature_FromIDA(_luaD_throw, "48 83 EC ? 44 8B C2 48 8B D1 48 8D 4C 24 ? E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 "
                                            "8D 4C 24 ? E8 ? ? ? ? CC CC CC");
        _MakeSignature_FromIDA(_luau_execute, "80 79 06 00 0F 85 ? ? ? ? E9 ? ? ? ? CC");
        _MakeSignature_FromIDA(_lua_pushvalue,
                               "48 89 5C 24 ? 57 48 83 EC ? F6 41 01 04 48 8B D9 48 63 FA 74 0C 4C 8D 41 ? 48 8B D1 E8 "
                               "? ? ? ? 85 FF 7E 24 48 8B 43 ? 48 8B CF 48 C1 E1 ? 48 83 C0 ? 48 03 C1 48 8B 4B ? 48 "
                               "3B C1 72 2F 48 8D 05 ? ? ? ? EB 26 81 FF F0 D8 FF FF 7E 10");
        _MakeSignature_FromIDA(_luaE_newthread,
                               "48 89 5C 24 ? 57 48 83 EC ? 44 0F B6 41 ? BA ? ? ? ? 48 8B F9 E8 ? ? ? "
                               "? 48 8B 57 ? 48 8B D8 44 0F B6 42 ? C6 00 ? 41 80 E0 ? 44 88 40 ?");
        _MakeSignature_FromIDA(_luaC_step, "48 8B 59 ? B8 ? ? ? ? 0F B6 F2 0F 29 74 24 ? 4C 8B F1 44 8B 43 ?");

        _MakeSignature_FromIDA(
                _luaD_rawrununprotected,
                "48 89 4C 24 ? 48 83 EC ? 48 8B C2 49 8B D0 FF D0 33 C0 EB 04 8B 44 24 48 48 83 C4 ? C3");

        _MakeSignature_FromIDA(_luaH_new,
                               "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B F0 8B "
                               "EA 44 0F B6 41 ? BA ? ? ? ? 48 8B F9 E8 ? ? ? ? 4C 8B 4F ? 48 8B D8 45 0F B6 "
                               "51 ? C6 00 ? 41 80 E2 ? 44 88 50 ? 44 0F B6 47 ? 44 88 40 ?");

        _MakeSignature_FromIDA(
                _freeblock,
                "4C 8B 51 ? 49 83 E8 ? 44 8B CA 4C 8B D9 49 8B 10 48 83 7A 28 00 75 22 83 7A 30 00 7D 1C 49 63 C1");

        _MakeSignature_FromIDA(_luaV_settable,
                               "48 89 5C 24 ? 48 89 6C 24 ? 56 41 54 41 57 48 83 EC ? 48 89 7C 24 ? 4D 8B E1 4C 89 74 "
                               "24 ? 4D 8B F8 48 8B F2 48 8B D9 33 ED 0F 1F 44 00 00 83 7E 0C 06 75 4C");

        _MakeSignature_FromIDA(
                _luaV_gettable,
                "48 89 5C 24 ? 55 41 54 41 55 41 56 41 57 48 83 EC ? 48 89 74 24 ? 4C 8D 2D ? ? ? ? 48 89 7C 24 ? 4D 8B E1 4D 8B F8 48 8B DA 4C 8B F1 33 ED 83 7B 0C 06 75 76");
        const static std::map<std::string, Signature> s_luauSignatureMap = {
                {"luaV_settable", _luaV_settable}, {"luaV_gettable", _luaV_gettable},
                {"luaD_throw", _luaD_throw},       {"luau_execute", _luau_execute},
                {"lua_pushvalue", _lua_pushvalue}, {"luaE_newthread", _luaE_newthread},
                {"luaC_step", _luaC_step},         {"luaD_rawrununprotected", _luaD_rawrununprotected},
                {"luaH_new", _luaH_new},           {"freeblock", _freeblock}};
        // TODO: Assess whether freeblock is required once again to be hooked due to stability issues.
    } // namespace LuauSignatures

#undef _MakeSignature_FromIDA
} // namespace RbxStu

static void luau__freeblock(lua_State *L, uint32_t sizeClass, void *block) {
    if (reinterpret_cast<std::uintptr_t>(block) > 0x00007FF000000000) {
        Logger::GetSingleton()->PrintWarning(
                RbxStu::HookedFunction,
                std::format("Suspicious address caught (non-heap range): {}. Deallocation blocked!", block));
        return;
    }
    if (!Utilities::IsPointerValid(static_cast<std::uintptr_t *>(block)) ||
        !Utilities::IsPointerValid(reinterpret_cast<std::uintptr_t **>(reinterpret_cast<std::uintptr_t>(block) - 8)) ||
        !Utilities::IsPointerValid(*reinterpret_cast<std::uintptr_t **>(reinterpret_cast<std::uintptr_t>(block) - 8))) {
        Logger::GetSingleton()->PrintWarning(
                RbxStu::HookedFunction, std::format("Suspicious address caught: {}. Deallocation blocked!", block));
        return;
    }

    return (reinterpret_cast<RbxStu::LuauFunctionDefinitions::freeblock>(
            LuauManager::GetSingleton()->GetHookOriginal("freeblock"))(L, sizeClass, block));
}

static void newThreadAfter(lua_State *newLuaThread) {
    Sleep(1000);
    auto *plStateUd = static_cast<RBX::Lua::ExtraSpace *>(newLuaThread->userdata);
    if (plStateUd->identity != 2)
        return;

    const auto logger = Logger::GetSingleton();

    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(newLuaThread) << " thread got identity "
       << std::dec << plStateUd->identity << " and capabilities 0x" << std::hex << std::uppercase
       << plStateUd->capabilities;

    logger->PrintInformation("RbxStu::luaE_newthread_hook", ss.str());
    const auto security = Security::GetSingleton();
    security->PrintCapabilities(plStateUd->capabilities);
}

static void *luaE__newthread(lua_State *on) {
    auto originalFunction = reinterpret_cast<RbxStu::LuauFunctionDefinitions::luaE_newthread>(
            LuauManager::GetSingleton()->GetHookOriginal("luaE_newthread"));
    auto newLuaThread = originalFunction(on);
    const auto logger = Logger::GetSingleton();

    std::stringstream ss;
    ss << "New lua thread got opened: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(newLuaThread);
    logger->PrintInformation("RbxStu::luaE_newthread_hook", ss.str());

    std::thread([newLuaThread]() { newThreadAfter(newLuaThread); }).detach();

    return newLuaThread;
}

static std::shared_mutex __luaumanager__singletonmutex;
std::shared_ptr<LuauManager> LuauManager::pInstance;

void LuauManager::Initialize() {
    const auto logger = Logger::GetSingleton();
    if (this->m_bIsInitialized) {
        logger->PrintWarning(RbxStu::LuauManager, "This instance is already initialized!");
        return;
    }
    const auto robloxManager = RobloxManager::GetSingleton();
    const auto scanner = Scanner::GetSingleton();

    logger->PrintInformation(RbxStu::LuauManager, "Initializing MinHook [1/4]");

    MH_Initialize(); // We don't care if we are already initialized, better to prevent any errors.

    logger->PrintInformation(RbxStu::LuauManager, "Initializing Luau Manager [1/4]");

    logger->PrintInformation(RbxStu::LuauManager, "Scanning functions (simple)... [1/4]");
    for (const auto &[fName, fSignature]: RbxStu::LuauSignatures::s_luauSignatureMap) {
        const auto results = scanner->Scan(fSignature);

        if (results.empty()) {
            logger->PrintWarning(RbxStu::LuauManager, std::format("Failed to find function '{}'!", fName));
        } else {
            void *mostDesirable = *results.data();
            if (results.size() > 1) {
                logger->PrintWarning(
                        RbxStu::LuauManager,
                        std::format(
                                "More than one candidate has matched the signature. This is generally not something "
                                "problematic, but it may mean the signature has too many wildcards that makes it not "
                                "unique. "
                                "The first result will be chosen. Affected function: {}",
                                fName));

                auto closest = *results.data();
                for (const auto &result: results) {
                    if (reinterpret_cast<std::uintptr_t>(result) -
                                reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr)) <
                        (reinterpret_cast<std::uintptr_t>(closest) -
                         reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr)))) {
                        mostDesirable = result;
                    }
                }
            }
            // Here we need to get a little bit smart. It is very possible we have MORE than one result.
            // To combat this, we want to grab the address that is closest to GetModuleHandleA(nullptr).
            // A mere attempt at making THIS, less painful.

            this->m_mapLuauFunctions[fName] =
                    mostDesirable; // Grab first result, it doesn't really matter to be honest.
        }
    }

    logger->PrintInformation(RbxStu::LuauManager, "Functions Found via simple scanning:");
    for (const auto &[funcName, funcAddress]: this->m_mapLuauFunctions) {
        logger->PrintInformation(RbxStu::LuauManager, std::format("- '{}' at address {}.", funcName, funcAddress));
    }


    logger->PrintInformation(RbxStu::LuauManager, "Overwriting .data pointers for RVM [2/4]");
    RbxStuOffsets::GetSingleton()->SetOffset("fireproximityprompt",
                                             robloxManager->GetRobloxFunction("RBX::ProximityPrompt::onTriggered"));


#define MapFunction(funcName, mappedName)                                                                              \
    RbxStuOffsets::GetSingleton()->SetOffset(mappedName, this->m_mapLuauFunctions[mappedName])
    MapFunction(luau_execute, "luau_execute");
    MapFunction(luaD_throw, "luaD_throw");
    MapFunction(luaE_newthread, "luaE_newthread");
    MapFunction(luaC_Step, "luaC_step");
    MapFunction(luaD_rawrununprotected, "luaD_rawrununprotected");
    MapFunction(luaV_gettable, "luaV_gettable");
    MapFunction(luaV_settable, "luaV_settable");
#undef MapFunction

    logger->PrintInformation(RbxStu::LuauManager, "Resolving data pointers to luaH_dummyNode and luaO_nilObject [3/4]");

    //  To obtain these data pointers, we must first obtain two key functions, lua_pushvalue and luaH_new
    //  Then we create a lua_State ourselves and we make calls that make them return to either the stack or to a data
    //  structure the pointer to them. This is by far the cleanest and easiest method, and avoids sins like crawling
    //  assembly and doing binary tomfoolery, or the upmost WORST choice like sigging .rdata or .data, which is insane.

    logger->PrintInformation(RbxStu::LuauManager, "Initializing lua_State for dumping");
    lua_State *luaState = luaL_newstate();

    {
        if (this->m_mapLuauFunctions["lua_pushvalue"] == nullptr) {
            logger->PrintError(RbxStu::LuauManager, "Failed to obtain lua_pushvalue, cannot resolve luaO_nilobject!");

            throw std::exception("Failed to resolve required function using signatures, cowardly refusing to complete "
                                 "LuauManagers' initialization step.");
        }

        logger->PrintInformation(RbxStu::LuauManager,
                                 std::format("Invoking lua_pushvalue(lua_State *L, int32_t idx) @ {} ...",
                                             this->m_mapLuauFunctions["lua_pushvalue"]));
        const auto offsets = RbxStuOffsets::GetSingleton();
        offsets->SetOffset("luaO_nilobject", static_cast<void *(__fastcall *) (lua_State * L, int32_t lua_index)>(
                                                     this->m_mapLuauFunctions["lua_pushvalue"])(luaState, 1));

        logger->PrintInformation(RbxStu::LuauManager,
                                 std::format("Resolved luaO_nilobject to pointer {}",
                                             reinterpret_cast<void *>(offsets->GetOffset("luaO_nilobject"))));

        const auto luaH_new = static_cast<void *(__fastcall *) (void *L, int32_t narray, int32_t nhash)>(
                this->m_mapLuauFunctions["luaH_new"]);

        if (luaH_new == nullptr) {
            logger->PrintError(RbxStu::LuauManager, "Failed to obtain luaH_new, cannot resolve luaH_dummyNode!");

            throw std::exception("Failed to resolve required function using signatures, cowardly refusing to complete "
                                 "LuauManagers' initialization step.");
        }

        logger->PrintInformation(RbxStu::LuauManager,
                                 std::format("Invoking luaH_new(lua_State *L, int32_t narray, int32_t nhash) @ {} ...",
                                             this->m_mapLuauFunctions["luaH_new"]));
        auto table = luaH_new(luaState, 0, 0);
        offsets->SetOffset("luaH_dummynode", (static_cast<Table *>(table)->node));

        logger->PrintInformation(RbxStu::LuauManager,
                                 std::format("Resolved luaH_dummyNode to pointer {}",
                                             reinterpret_cast<void *>(offsets->GetOffset("luaH_dummynode"))));
    }

    logger->PrintInformation(RbxStu::LuauManager, "Cleaning up lua_State used to obtain values... ");
    lua_close(luaState);
    logger->PrintInformation(RbxStu::LuauManager, "All cleaned up!");

    logger->PrintInformation(RbxStu::LuauManager, "Hooking functions... [3/4]");

    logger->PrintInformation(RbxStu::LuauManager, "- Installing pointer check hook into freeblock...");
    this->m_mapHookMap["freeblock"] = new void *();

    // Error checking, because Dottik didn't add it.
    // - MakeSureDudeDies
    if (MH_CreateHook(this->m_mapLuauFunctions["freeblock"], luau__freeblock, &this->m_mapHookMap["freeblock"]) !=
        MH_OK) {
        logger->PrintError(RbxStu::LuauManager, "Failed to create freeblock hook!");
        throw std::exception("Creating freeblock hook failed.");
    }

    if (MH_EnableHook(this->m_mapLuauFunctions["freeblock"]) != MH_OK) {
        logger->PrintError(RbxStu::LuauManager, "Failed to enable freeblock hook!");
        throw std::exception("Enabling freeblock hook failed.");
    }

    // this->m_mapHookMap["luaE_newthread"] = new void *();
    // MH_CreateHook(this->m_mapLuauFunctions["luaE_newthread"], luaE__newthread,
    // &this->m_mapHookMap["luaE_newthread"]); MH_EnableHook(this->m_mapLuauFunctions["luaE_newthread"]);

    logger->PrintInformation(RbxStu::LuauManager, "Initialization completed [4/4]");
    this->m_bIsInitialized = true;
}

std::shared_ptr<LuauManager> LuauManager::GetSingleton() {
    std::lock_guard lock{__luaumanager__singletonmutex};
    if (LuauManager::pInstance == nullptr)
        LuauManager::pInstance = std::make_shared<LuauManager>();


    if (!LuauManager::pInstance->m_bIsInitialized)
        LuauManager::pInstance->Initialize();

    return LuauManager::pInstance;
}
bool LuauManager::IsInitialized() const { return this->m_bIsInitialized; }

void *LuauManager::GetHookOriginal(const std::string &functionName) {
    if (this->m_mapHookMap.contains(functionName)) {
        return this->m_mapHookMap[functionName];
    }

    if (this->m_mapLuauFunctions.contains(functionName)) {
        return this->m_mapLuauFunctions[functionName];
    }

    return nullptr;
}
void *LuauManager::GetFunction(const std::string &functionName) { return this->m_mapLuauFunctions[functionName]; }
