//
// Created by Dottik on 26/8/2024.
//

#include "DisassembledChunk.hpp"
DisassembledChunk::DisassembledChunk(cs_insn *pInstructions, std::size_t ullInstructionCount) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < ullInstructionCount; i++) {
        this->vInstructionsvec.push_back(*(pInstructions + i));
    }

    free(pInstructions);
}

bool DisassembledChunk::ContainsInstruction(const char *szMnemonic, const char *szOperationAsString,
                                            bool bUseContains) {
    for (const auto &instr: this->vInstructionsvec) {
        if (!bUseContains && strcmp(instr.mnemonic, szMnemonic) == 0 &&
            strcmp(instr.op_str, szOperationAsString) == 0) {
            return true;
        }

        if (bUseContains && strstr(instr.mnemonic, szMnemonic) != nullptr &&
            strstr(instr.op_str, szOperationAsString) != nullptr) {
            return true;
        }
    }
}

std::vector<cs_insn> DisassembledChunk::GetInstructions() { return this->vInstructionsvec; }
