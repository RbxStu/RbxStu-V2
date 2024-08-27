//
// Created by Dottik on 26/8/2024.
//
#pragma once

#include <array>
#include <capstone/capstone.h>
#include <vector>

class DisassembledChunk final {
    std::vector<cs_insn> vInstructionsvec;
    cs_insn *originalInstruction;
    std::size_t instructionCount;

public:
    ~DisassembledChunk();
    DisassembledChunk(_In_ cs_insn *pInstructions, std::size_t ullInstructionCount);

    bool ContainsInstruction(_In_ const char *szMnemonic, _In_ const char *szOperationAsString, bool bUseContains);
    bool ContainsInstructionChain(std::vector<const char *> szMnemonics, std::vector<const char *> szOperationAsString,
                                  bool bUseContains);

    std::vector<cs_insn> GetInstructions();
};
