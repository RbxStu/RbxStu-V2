# CS_ARCH_AARCH64, 0, None

0xa0,0x60,0x38,0xd5 == mrs x0, PFAR_EL1
0xa0,0x60,0x18,0xd5 == msr PFAR_EL1, x0
0xa0,0x60,0x3c,0xd5 == mrs x0, PFAR_EL2
0xa0,0x60,0x1c,0xd5 == msr PFAR_EL2, x0
0xa0,0x60,0x3d,0xd5 == mrs x0, PFAR_EL12
0xa0,0x60,0x1d,0xd5 == msr PFAR_EL12, x0
0xa0,0x60,0x3e,0xd5 == mrs x0, MFAR_EL3
0xa0,0x60,0x1e,0xd5 == msr MFAR_EL3, x0
