//
// Created by Dottik on 26/8/2024.
//
#pragma once

#include <capstone/capstone.h>
#include <vector>

class DisassembledChunk final {
    std::vector<cs_insn> vInstructionsvec;

public:
    DisassembledChunk(_In_ cs_insn *pInstructions, std::size_t ullInstructionCount);

    bool ContainsInstruction(const char *szMnemonic, const char *szOperationAsString, bool bUseContains);

    std::vector<cs_insn> GetInstructions();
};
