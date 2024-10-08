#!/usr/bin/env python3

# Capstone Python bindings, by Dmitry Sibirtsev <sibirtsev_dl@gmail.com>

from capstone import *
from capstone.hppa import *
from xprint import to_x, to_hex

HPPA_20_CODE_BE = b'\x00\x20\x50\xa2\x00\x01\x58\x20\x00\x00\x44\xa1\x00\x41\x18\x40\x00\x20\x08\xa2\x01\x60\x48\xa1\x01\x61\x18\xc0\x00\x00\x14\xa1\x00\x0f\x0d\x61\x00\x0f\x0e\x61\x00\x01\x18\x60\x00\x00\x0c\x00\x00\x00\x0c\xa0\x03\xff\xc0\x1f\x00\x00\x04\x00\x00\x10\x04\x00\x04\x22\x51\x83\x04\x22\x51\xc3\x04\x22\x51\x83\x04\x2f\x71\x83\x04\x2f\x71\xc3\x04\x2f\x71\x83\x04\x41\x53\x43\x04\x41\x53\x63\x04\x41\x53\x03\x04\x41\x12\x00\x04\x41\x16\x00\x04\x41\x16\x20\x04\x41\x42\x00\x04\x41\x46\x00\x04\x41\x46\x20\x04\x41\x12\x40\x04\x41\x12\x60\x04\x41\x42\x40\x04\x41\x42\x60\x04\x41\x18\x00\x04\x41\x08\x00\x04\x41\x13\x80\x04\x41\x13\xa0\x04\x41\x52\x80\x04\x41\x52\xa0\x04\x5e\x72\x80\x04\x41\x42\x80\x04\x41\x52\xc0\x04\x41\x52\xe0\x04\x41\x42\xc0\x04\x41\x42\xe0\x14\x00\xde\xad'
HPPA_20_CODE =    b'\xa2\x50\x20\x00\x20\x58\x01\x00\xa1\x44\x00\x00\x40\x18\x41\x00\xa2\x08\x20\x00\xa1\x48\x60\x01\xc0\x18\x61\x01\xa1\x14\x00\x00\x61\x0d\x0f\x00\x61\x0e\x0f\x00\x60\x18\x01\x00\x00\x0c\x00\x00\xa0\x0c\x00\x00\x1f\xc0\xff\x03\x00\x04\x00\x00\x00\x04\x10\x00\x83\x51\x22\x04\xc3\x51\x22\x04\x83\x51\x22\x04\x83\x71\x2f\x04\xc3\x71\x2f\x04\x83\x71\x2f\x04\x43\x53\x41\x04\x63\x53\x41\x04\x03\x53\x41\x04\x00\x12\x41\x04\x00\x16\x41\x04\x20\x16\x41\x04\x00\x42\x41\x04\x00\x46\x41\x04\x20\x46\x41\x04\x40\x12\x41\x04\x60\x12\x41\x04\x40\x42\x41\x04\x60\x42\x41\x04\x00\x18\x41\x04\x00\x08\x41\x04\x80\x13\x41\x04\xa0\x13\x41\x04\x80\x52\x41\x04\xa0\x52\x41\x04\x80\x72\x5e\x04\x80\x42\x41\x04\xc0\x52\x41\x04\xe0\x52\x41\x04\xc0\x42\x41\x04\xe0\x42\x41\x04\xad\xde\x00\x14'
HPPA_11_CODE_BE = b'\x24\x41\x40\xc3\x24\x41\x60\xc3\x24\x41\x40\xe3\x24\x41\x60\xe3\x24\x41\x68\xe3\x2c\x41\x40\xc3\x2c\x41\x60\xc3\x2c\x41\x40\xe3\x2c\x41\x60\xe3\x2c\x41\x68\xe3\x24\x62\x42\xc1\x24\x62\x62\xc1\x24\x62\x42\xe1\x24\x62\x46\xe1\x24\x62\x62\xe1\x24\x62\x6a\xe1\x2c\x62\x42\xc1\x2c\x62\x62\xc1\x2c\x62\x42\xe1\x2c\x62\x46\xe1\x2c\x62\x62\xe1\x2c\x62\x6a\xe1\x24\x3e\x50\xc2\x24\x3e\x50\xe2\x24\x3e\x70\xe2\x24\x3e\x78\xe2\x2c\x3e\x50\xc2\x2c\x3e\x50\xe2\x2c\x3e\x70\xe2\x2c\x3e\x78\xe2\x24\x5e\x52\xc1\x24\x5e\x52\xe1\x24\x5e\x56\xe1\x24\x5e\x72\xe1\x24\x5e\x7a\xe1\x2c\x5e\x52\xc1\x2c\x5e\x52\xe1\x2c\x5e\x56\xe1\x2c\x5e\x72\xe1\x2c\x5e\x7a\xe1'
HPPA_11_CODE =    b'\xc3\x40\x41\x24\xc3\x60\x41\x24\xe3\x40\x41\x24\xe3\x60\x41\x24\xe3\x68\x41\x24\xc3\x40\x41\x2c\xc3\x60\x41\x2c\xe3\x40\x41\x2c\xe3\x60\x41\x2c\xe3\x68\x41\x2c\xc1\x42\x62\x24\xc1\x62\x62\x24\xe1\x42\x62\x24\xe1\x46\x62\x24\xe1\x62\x62\x24\xe1\x6a\x62\x24\xc1\x42\x62\x2c\xc1\x62\x62\x2c\xe1\x42\x62\x2c\xe1\x46\x62\x2c\xe1\x62\x62\x2c\xe1\x6a\x62\x2c\xc2\x50\x3e\x24\xe2\x50\x3e\x24\xe2\x70\x3e\x24\xe2\x78\x3e\x24\xc2\x50\x3e\x2c\xe2\x50\x3e\x2c\xe2\x70\x3e\x2c\xe2\x78\x3e\x2c\xc1\x52\x5e\x24\xe1\x52\x5e\x24\xe1\x56\x5e\x24\xe1\x72\x5e\x24\xe1\x7a\x5e\x24\xc1\x52\x5e\x2c\xe1\x52\x5e\x2c\xe1\x56\x5e\x2c\xe1\x72\x5e\x2c\xe1\x7a\x5e\x2c'

all_tests = (
    (CS_ARCH_HPPA, CS_MODE_BIG_ENDIAN | CS_MODE_HPPA_20, HPPA_20_CODE_BE, "HPPA 2.0 (Big-endian)"),
    (CS_ARCH_HPPA, CS_MODE_LITTLE_ENDIAN | CS_MODE_HPPA_20, HPPA_20_CODE, "HPPA 2.0 (Little-endian)"),
    (CS_ARCH_HPPA, CS_MODE_BIG_ENDIAN | CS_MODE_HPPA_11, HPPA_11_CODE_BE, "HPPA 1.1 (Big-endian)"),
    (CS_ARCH_HPPA, CS_MODE_LITTLE_ENDIAN | CS_MODE_HPPA_11, HPPA_11_CODE, "HPPA 1.1 (Little-endian)"),
)


def print_insn_detail(insn):
    # print address, mnemonic and operands
    print("0x%x:\t%s\t%s" % (insn.address, insn.mnemonic, insn.op_str))

    # "data" instruction generated by SKIPDATA option has no detail
    if insn.id == 0:
        return

    if len(insn.operands) > 0:
        print("\top_count: %u" % len(insn.operands))
        c = 0
        for i in insn.operands:
            if i.type == HPPA_OP_REG:
                print("\t\toperands[%u].type: REG = %s" % (c, insn.reg_name(i.reg)))
            if i.type == HPPA_OP_IMM:
                print("\t\toperands[%u].type: IMM = 0x%s" % (c, to_x(i.imm)))
            if i.type == HPPA_OP_IDX_REG:
                print("\t\toperands[%u].type: IDX_REG = %s" % (c, insn.reg_name(i.reg)))
            if i.type == HPPA_OP_DISP:
                print("\t\toperands[%u].type: DISP = 0x%s" % (c, to_x(i.imm)))
            if i.type == HPPA_OP_MEM:
                print("\t\toperands[%u].type: MEM" % c)
                if i.mem.space != HPPA_REG_INVALID:
                    print("\t\t\toperands[%u].mem.space: REG = %s" % (c, insn.reg_name(i.mem.space)))
                print("\t\t\toperands[%u].mem.base: REG = %s" % (c, insn.reg_name(i.mem.base)))
            if i.type == HPPA_OP_TARGET:
                if i.imm >= 0x8000000000000000:
                    print("TARGET = -0x%lx" % i.imm)
                else:
                    print("TARGET = 0x%lx" % i.imm)
            c += 1

# ## Test class Cs
def test_class():
    for (arch, mode, code, comment) in all_tests:
        print("*" * 16)
        print("Platform: %s" % comment)
        print("Code: %s" % to_hex(code))
        print("Disasm:")

        try:
            md = Cs(arch, mode)
            md.detail = True
            for insn in md.disasm(code, 0x1000):
                print_insn_detail(insn)
                print()
            print("0x%x:\n" % (insn.address + insn.size))
        except CsError as e:
            print("ERROR: %s" % e)


if __name__ == '__main__':
    test_class()
