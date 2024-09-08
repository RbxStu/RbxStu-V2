//
// Created by Dottik on 26/8/2024.
//

#include "DisassembledChunk.hpp"

#include <optional>

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
        if (!bUseContains && (!szMnemonic || strcmp(instr.mnemonic, szMnemonic) == 0) &&
            (!szOperationAsString || strcmp(instr.op_str, szOperationAsString) == 0)) {
            return true;
        }

        if (bUseContains && (!szMnemonic || strstr(instr.mnemonic, szMnemonic) != nullptr) &&
            (!szOperationAsString || strstr(instr.op_str, szOperationAsString) != nullptr)) {
            return true;
        }
    }

    return false;
}
std::optional<const cs_insn> DisassembledChunk::GetInstructionWhichMatches(const char *szMnemonic,
                                                                           const char *szOperationAsString,
                                                                           bool bUseContains) {
    for (const auto &instr: this->vInstructionsvec) {
        if (!bUseContains && (szMnemonic && strcmp(instr.mnemonic, szMnemonic) == 0 || !szMnemonic) &&
            (szOperationAsString && strcmp(instr.op_str, szOperationAsString) == 0 || !szOperationAsString)) {
            return instr;
        }

        if (bUseContains && (szMnemonic && strstr(instr.mnemonic, szMnemonic) != nullptr || !szMnemonic) &&
            (szOperationAsString && strstr(instr.op_str, szOperationAsString) != nullptr || !szOperationAsString)) {
            return instr;
        }
    }

    return {};
}

std::vector<cs_insn> DisassembledChunk::GetInstructions() { return this->vInstructionsvec; }
std::string DisassembledChunk::RenderInstructions() {
    std::stringstream strstream{};

    for (const auto &insn: this->GetInstructions()) {
        strstream << std::format("{}"
                                 ":\t{}\t\t{}\n",
                                 reinterpret_cast<void *>(insn.address), insn.mnemonic, insn.op_str);
    }

    return strstream.str();
}
