# CS_ARCH_AARCH64, 0, None

0x89,0x03,0x38,0xd5 == mrs x9, {{id_pfr2_el1|ID_PFR2_EL1}}
0xe8,0xd0,0x3b,0xd5 == mrs x8, {{scxtnum_el0|SCXTNUM_EL0}}
0xe7,0xd0,0x38,0xd5 == mrs x7, {{scxtnum_el1|SCXTNUM_EL1}}
0xe6,0xd0,0x3c,0xd5 == mrs x6, {{scxtnum_el2|SCXTNUM_EL2}}
0xe5,0xd0,0x3e,0xd5 == mrs x5, {{scxtnum_el3|SCXTNUM_EL3}}
0xe4,0xd0,0x3d,0xd5 == mrs x4, {{scxtnum_el12|SCXTNUM_EL12}}
0xe8,0xd0,0x1b,0xd5 == msr {{scxtnum_el0|SCXTNUM_EL0}},   x8
0xe7,0xd0,0x18,0xd5 == msr {{scxtnum_el1|SCXTNUM_EL1}},   x7
0xe6,0xd0,0x1c,0xd5 == msr {{scxtnum_el2|SCXTNUM_EL2}},   x6
0xe5,0xd0,0x1e,0xd5 == msr {{scxtnum_el3|SCXTNUM_EL3}},   x5
0xe4,0xd0,0x1d,0xd5 == msr {{scxtnum_el12|SCXTNUM_EL12}}, x4
