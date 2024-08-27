//
// Created by Dottik on 26/8/2024.
//

#include "DisassembledChunk.hpp"

#include "Logger.hpp"
DisassembledChunk::~DisassembledChunk() { cs_free(this->originalInstruction, this->instructionCount); }
DisassembledChunk::DisassembledChunk(cs_insn *pInstructions, std::size_t ullInstructionCount) {
    std::size_t count = 0;
    this->vInstructionsvec.reserve(ullInstructionCount);
    for (std::size_t i = 0; i < ullInstructionCount; i++) {
        this->vInstructionsvec.push_back(*(pInstructions + i));
    }

    this->instructionCount = ullInstructionCount;
    this->originalInstruction = pInstructions;
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

bool DisassembledChunk::ContainsInstructionChain(std::vector<const char *>& szMnemonics,
                                                 std::vector<const char *>& szOperationAsString, bool bUseContains) {
    const auto logger = Logger::GetSingleton();
    if (szMnemonics.size() != szOperationAsString.size()) {
        logger->PrintError(RbxStu::Anonymous, "Cannot determine if the DisassembledChunk contains the chain of "
                                              "instructions. Reason: szMnemonics.size() != szOperationAsString.size(); "
                                              "making it impossible to compute.");
        return false;
    }
    for (const auto &instr: this->vInstructionsvec) {
        auto matchCount = 0;
        for (std::size_t i = 0; i < szMnemonics.size(); i++) {
            if (((!bUseContains && strcmp(instr.mnemonic, szMnemonics[i]) == 0) ||
                 bUseContains && strstr(instr.mnemonic, szMnemonics[i]) != nullptr) &&
                ((!bUseContains && strcmp(instr.op_str, szOperationAsString[i]) == 0) ||
                 bUseContains && strstr(instr.op_str, szOperationAsString[i]) != nullptr)) {
                matchCount++;
            }
        }
        if (matchCount == szMnemonics.size())
            return true;
    }

    return false;
}

const std::vector<cs_insn>& DisassembledChunk::GetInstructions() { return this->vInstructionsvec; }
