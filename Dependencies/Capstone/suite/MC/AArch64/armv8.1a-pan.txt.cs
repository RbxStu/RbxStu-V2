# CS_ARCH_AARCH64, None, None
# This regression test file is new. The option flags could not be determined.
# LLVM uses the following mattr = ['mattr=+v8.1a']
0x9f,0x40,0x00,0xd5 == msr PAN, #0
0x9f,0x41,0x00,0xd5 == msr PAN, #1
0x9f,0x4f,0x00,0xd5 == msr PAN, #15
0x65,0x42,0x18,0xd5 == msr PAN, x5
0x6d,0x42,0x38,0xd5 == mrs x13, PAN
