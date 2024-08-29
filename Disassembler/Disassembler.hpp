//
// Created by Dottik on 26/8/2024.
//
#pragma once
#include <complex.h>

#include <map>
#include <memory>
#include <optional>

#include <capstone/capstone.h>

#include "DisassembledChunk.hpp"

struct DisassemblyRequest;
/// @brief An abstraction for Disassembling x86_64 code using capstone as a base!
class Disassembler final {
    static std::shared_ptr<Disassembler> pInstance;
    bool m_bIsInitialized = false;
    csh m_pCapstoneHandle{};
    void Initialize();

public:
    static std::shared_ptr<Disassembler> GetSingleton();

    std::optional<std::unique_ptr<DisassembledChunk>> GetInstructions(_In_ DisassemblyRequest &disassemblyRequest);
    std::optional<void *> TranslateRelativeLeaIntoRuntimeAddress(const cs_insn &insn);
    void * ObtainPossibleEndFromStart(void * mapped);
};
