//
// Created by Dottik on 26/8/2024.
//


#include "Disassembler.hpp"

#include <Windows.h>
#include <capstone/capstone.h>
#include <optional>

#include "DisassemblyRequest.hpp"
#include "Logger.hpp"

std::shared_ptr<Disassembler> Disassembler::pInstance;

static std::mutex __initlock;

void Disassembler::Initialize() {
    std::scoped_lock lg{__initlock};
    const auto logger = Logger::GetSingleton();

    if (this->m_bIsInitialized) {
        logger->PrintWarning(RbxStu::Disassembler, "Initialize has been called on an already initialized instance.");
        return;
    }

    logger->PrintInformation(RbxStu::Disassembler, "Initializing capstone - The ultimate disassembler!");

    if (auto status = cs_open(cs_arch::CS_ARCH_X86, cs_mode::CS_MODE_64, &this->m_pCapstoneHandle);
        status != cs_err::CS_ERR_OK) {
        logger->PrintError(RbxStu::Disassembler, std::format("Failed to initialize capstone! Error code: {:#x}",
                                                             static_cast<int64_t>(status)));
        throw std::exception("cannot initialize disassembler. Reason: capstone couldn't be initialized!");
    }

    cs_option(this->m_pCapstoneHandle, CS_OPT_DETAIL, CS_OPT_ON);

    logger->PrintInformation(RbxStu::Disassembler,
                             std::format("Capstone initialized successfully. Capstone Context: {}",
                                         reinterpret_cast<void *>(&this->m_pCapstoneHandle)));

    this->m_bIsInitialized = true;

    logger->PrintInformation(RbxStu::Disassembler, "Disassembler initialized successfully!");
}

std::shared_ptr<Disassembler> Disassembler::GetSingleton() {
    if (Disassembler::pInstance == nullptr)
        pInstance = std::make_shared<Disassembler>();

    if (!Disassembler::pInstance->m_bIsInitialized)
        Disassembler::pInstance->Initialize();

    return Disassembler::pInstance;
}
std::optional<std::unique_ptr<DisassembledChunk>>
Disassembler::GetInstructions(_In_ DisassemblyRequest &disassemblyRequest) {
    const auto logger = Logger::GetSingleton();
    // Not true, it's mutated on Initialized, lol, stupid ReSharper.
    // ReSharper disable once CppDFAConstantConditions
    if (!this->m_bIsInitialized) {
        logger->PrintWarning(RbxStu::Disassembler,
                             "Cannot comply with disassembly request: Disassembler not initialized!");
        return {};
    }
    // FFS ITS NOT UNREACHABLE DUDE.
    // ReSharper disable once CppDFAUnreachableCode

    if (disassemblyRequest.pStartAddress < disassemblyRequest.pEndAddress) {
        logger->PrintDebug(RbxStu::Disassembler, "pStartAddress is bigger than pEndAddress! For the purposes of "
                                                 "simplicity, the addresses will be flipped! Beware!");
        const auto t = disassemblyRequest.pStartAddress;
        disassemblyRequest.pStartAddress = disassemblyRequest.pEndAddress;
        disassemblyRequest.pEndAddress = t;
    }

    const auto segmentSize = reinterpret_cast<std::uintptr_t>(disassemblyRequest.pStartAddress) -
                             reinterpret_cast<std::uintptr_t>(disassemblyRequest.pEndAddress);

    if (!disassemblyRequest.bIgnorePageProtection) {
#define CHECK_NOT_FLAG(num, flag) ((num & flag) != flag)
        MEMORY_BASIC_INFORMATION buf{nullptr};
        VirtualQuery(disassemblyRequest.pStartAddress, &buf, sizeof(buf));
        // ReSharper disable once CppRedundantComplexityInComparison
        if (CHECK_NOT_FLAG(buf.Protect, PAGE_EXECUTE) && CHECK_NOT_FLAG(buf.Protect, PAGE_EXECUTE_READ) &&
            CHECK_NOT_FLAG(buf.Protect, PAGE_EXECUTE_READWRITE) && CHECK_NOT_FLAG(buf.Protect, PAGE_GRAPHICS_EXECUTE)) {
            logger->PrintDebug(RbxStu::Disassembler,
                               "Memory protections are non-executable! Disassembly will not proceed.");
            return {};
        }
#undef CHECK_NOT_FLAG
    }

    logger->PrintDebug(RbxStu::Disassembler,
                       std::format("Disassembling segment: {} ~ {}. Size: {:#x}", disassemblyRequest.pStartAddress,
                                   disassemblyRequest.pEndAddress, segmentSize));

    cs_insn *instructions{0};

    auto disassembledCount = cs_disasm(
            this->m_pCapstoneHandle,
            static_cast<const uint8_t *>(const_cast<const void *>(disassemblyRequest.pStartAddress)), segmentSize,
            reinterpret_cast<std::uintptr_t>(disassemblyRequest.pStartAddress), segmentSize, &instructions);

    if (disassembledCount > 0) {
        logger->PrintDebug(RbxStu::Disassembler, "Serializing instructions into a DiassembledChunk instance!");
        return std::make_unique<DisassembledChunk>(instructions, disassembledCount);
    }

    logger->PrintError(RbxStu::Disassembler, "Failed to disassemble the given code!");
    return {};
}

std::optional<void *> Disassembler::TranslateRelativeLeaIntoRuntimeAddress(const cs_insn &insn) {
    if (strcmp(insn.mnemonic, "lea") != 0) { // Not lea.
        return {};
    }
    auto insnOpAsString = std::string(insn.op_str);
    if (insnOpAsString.find("rip") == std::string::npos) { // Not relative
        return {};
    }
    const auto ripOff =

            insnOpAsString.find("rip + 0x") + sizeof("rip + 0x") - 1; // stRIP null-byte. (SEE WHAT I DID THERE????)
    insnOpAsString = insnOpAsString.substr(ripOff, insnOpAsString.size() - ripOff);
    insnOpAsString = insnOpAsString.substr(0, insnOpAsString.size() - 1); // Strip last ']'
    char *endChar;

    return reinterpret_cast<void *>(insn.address + insn.size + strtoull(insnOpAsString.c_str(), &endChar, 16));
}

void *Disassembler::ObtainPossibleEndFromStart(void *mapped) {
    const auto logger = Logger::GetSingleton();
    // Compilers normally leave some stub 0xCC at the end of functions to split them up, I'm not joking.
    // we can abuse this to find the possible ending of a function, keyword, possible, we cannot really get everything
    // we want on this life :(. That also said, for the purposes of MORE simplicity, we will make the address an even
    // one, just for the sake of god.
    if (reinterpret_cast<std::uintptr_t>(mapped) % 2 != 0) {
        logger->PrintWarning(RbxStu::Disassembler, "Rounding address to a multiple of 2 for simplicity");
        mapped = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(mapped) - 1);
    }

    auto pAsm = static_cast<std::uint8_t *>(mapped);

    bool f = false;
    while (*pAsm == static_cast<std::uint8_t>(0xCC)) {
        if (!f) {
            logger->PrintWarning(
                    RbxStu::Disassembler,
                    "mapped has been dereferenced into a 0xCC! Assuming that the provided pointer is pointing to the "
                    "prelude of a function. Advancing mapped until dereferencing is not 0xCC!");
            f = true;
        }
        // The provided address is at the end of a function, we must crawl forward until we get to a valid code segment.
        pAsm++;
    }

    // We're now at the start of the next function, hopefully what the caller wants!

    while (*pAsm != static_cast<std::uint8_t>(0xCC) && pAsm++) {
        _mm_pause();
    }

    return pAsm;
}
