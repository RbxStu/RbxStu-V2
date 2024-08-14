//
// Created by Dottik on 13/8/2024.
//

#include "LuauManager.hpp"

#include <MinHook.h>
#include <StudioOffsets.h>
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
        using freeblock = void(__fastcall *)(lua_State *L, int32_t sizeClass, void *block);
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
                _luaD_rawrunprotected,
                "48 89 4C 24 ? 48 83 EC ? 48 8B C2 49 8B D0 FF D0 33 C0 EB 04 8B 44 24 48 48 83 C4 ? C3");

        _MakeSignature_FromIDA(_luaH_new,
                               "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B F0 8B "
                               "EA 44 0F B6 41 ? BA ? ? ? ? 48 8B F9 E8 ? ? ? ? 4C 8B 4F ? 48 8B D8 45 0F B6 "
                               "51 ? C6 00 ? 41 80 E2 ? 44 88 50 ? 44 0F B6 47 ? 44 88 40 ?");

        _MakeSignature_FromIDA(
                _freeblock,
                "4C 8B 51 ? 49 83 E8 ? 44 8B CA 4C 8B D9 49 8B 10 48 83 7A 28 00 75 22 83 7A 30 00 7D 1C 49 63 C1");

        const static std::map<std::string, Signature> s_luauSignatureMap = {
                {"luaD_throw", _luaD_throw},       {"luau_execute", _luau_execute},
                {"lua_pushvalue", _lua_pushvalue}, {"luaE_newthread", _luaE_newthread},
                {"luaC_step", _luaC_step},         {"luaD_rawrunprotected", _luaD_rawrunprotected},
                {"luaH_new", _luaH_new},           {"freeblock", _freeblock}};
        // TODO: Assess whether freeblock is required once again to be hooked due to stability issues.
    } // namespace LuauSignatures

#undef _MakeSignature_FromIDA
} // namespace RbxStu

template<typename T>
static bool is_pointer_valid(T *tValue) {
    // Templates fuck themselves if you don't have it like this lol

    const auto ptr = reinterpret_cast<void *>(tValue);
    auto buf = MEMORY_BASIC_INFORMATION{};

    // Query a full page.
    if (const auto read = VirtualQuery(ptr, &buf, sizeof(buf)); read != 0 && sizeof(buf) != read) {
        // I honestly dont care.
    } else if (read == 0) {
        return false;
    }

    if (buf.State & MEM_FREE == MEM_FREE) {
        return false; // The memory is not owned by the process, no need to do anything, we can already assume
        // we cannot read it.
    }

    auto validProtections = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                            PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

    if (buf.Protect & validProtections) {
        return true;
    }
    if (buf.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
        return false;
    }

    return true;
}


static void luau__freeblock(lua_State *L, uint32_t sizeClass, void *block) {
    if (reinterpret_cast<std::uintptr_t>(block) > 0x00007FF000000000)
        return;

    if (!is_pointer_valid(static_cast<std::uintptr_t *>(block)) ||
        !is_pointer_valid(reinterpret_cast<std::uintptr_t **>(reinterpret_cast<std::uintptr_t>(block) - 8)) ||
        !is_pointer_valid(*reinterpret_cast<std::uintptr_t **>(reinterpret_cast<std::uintptr_t>(block) - 8)))
        return;

    return (reinterpret_cast<RbxStu::LuauFunctionDefinitions::freeblock>(
            LuauManager::GetSingleton()->GetHookOriginal("freeblock"))(L, sizeClass, block));
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
    RBX::Studio::Offsets::fireproximityprompt =
            reinterpret_cast<std::uintptr_t>(robloxManager->GetRobloxFunction("RBX::ProximityPrompt::onTriggered"));

#define MapFunction(funcName, mappedName)                                                                              \
    RBX::Studio::Offsets::funcName = reinterpret_cast<std::uintptr_t>(this->m_mapLuauFunctions[mappedName]);           \
    RBX::Studio::Functions::funcName =                                                                                 \
            reinterpret_cast<RBX::Studio::FunctionTypes::funcName>(RBX::Studio::Offsets::funcName)
    MapFunction(luau_execute, "luau_execute");
    MapFunction(luaD_throw, "luaD_throw");
    MapFunction(luaE_newthread, "luaE_newthread");
    MapFunction(luaC_Step, "luaC_step");
    MapFunction(luaD_rawrununprotected, "luaD_rawrununprotected");
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
        RBX::Studio::Offsets::_luaO_nilobject =
                static_cast<std::uintptr_t(__fastcall *)(lua_State * L, int32_t lua_index)>(
                        this->m_mapLuauFunctions["lua_pushvalue"])(luaState, 1);

        logger->PrintInformation(RbxStu::LuauManager,
                                 std::format("Resolved luaO_nilobject to pointer {}",
                                             reinterpret_cast<void *>(RBX::Studio::Offsets::_luaO_nilobject)));

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
        RBX::Studio::Offsets::_luaH_dummynode = reinterpret_cast<std::uintptr_t>(static_cast<Table *>(table)->node);
        logger->PrintInformation(RbxStu::LuauManager,
                                 std::format("Resolved luaH_dummyNode to pointer {}",
                                             reinterpret_cast<void *>(RBX::Studio::Offsets::_luaH_dummynode)));
    }

    logger->PrintInformation(RbxStu::LuauManager, "Cleaning up lua_State used to obtain values... ");
    lua_close(luaState);
    logger->PrintInformation(RbxStu::LuauManager, "All cleaned up!");

    logger->PrintInformation(RbxStu::LuauManager, "Hooking functions... [3/4]");

    logger->PrintInformation(RbxStu::LuauManager, "- Installing pointer check hook into freeblock...");
    this->m_mapHookMap["freeblock"] = new void *();
    MH_CreateHook(this->m_mapLuauFunctions["freeblock"], luau__freeblock, &this->m_mapLuauFunctions["freeblock"]);
    MH_EnableHook(this->m_mapLuauFunctions["freeblock"]);
    logger->PrintInformation(RbxStu::LuauManager, "- Hook installed!");


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

void *LuauManager::GetHookOriginal(const std::string &functionName) {
    if (this->m_mapHookMap.contains(functionName)) {
        return this->m_mapHookMap[functionName];
    }

    return nullptr;
}
