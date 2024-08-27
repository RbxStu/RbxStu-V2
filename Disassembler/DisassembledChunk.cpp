//
// Created by Dottik on 26/8/2024.
//

#include "DisassembledChunk.hpp"
DisassembledChunk::~DisassembledChunk() { cs_free(this->originalInstruction, this->instructionCount); }
DisassembledChunk::DisassembledChunk(cs_insn *pInstructions, std::size_t ullInstructionCount) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < ullInstructionCount; i++) {
        this->vInstructionsvec.push_back(*(pInstructions + i));
    }

    // Freeing correctly will result in freeing pInstructions->detail, which we do not want,
    // but we also do not want to free other things like the pInstructions, because else we CANNOT free detail later on.
    // cs_free is to run when deconstructing for simplicity.
    // cs_free(pInstructions, ullInstructionCount);
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
