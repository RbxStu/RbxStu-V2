# CS_ARCH_AARCH64, None, None
# This regression test file is new. The option flags could not be determined.
# LLVM uses the following mattr = []
0x1f 0x20 0x03 0xd5 == nop
0x9f 0x20 0x03 0xd5 == sev
0xbf 0x20 0x03 0xd5 == sevl
0x5f 0x20 0x03 0xd5 == wfe
0x7f 0x20 0x03 0xd5 == wfi
0x3f 0x20 0x03 0xd5 == yield
0x5f 0x3a 0x03 0xd5 == clrex #10
0xdf 0x3f 0x03 0xd5 == isb{{$}}
0xdf 0x31 0x03 0xd5 == isb #1
0xbf 0x33 0x03 0xd5 == dmb osh
0x9f 0x37 0x03 0xd5 == dsb nsh
0x3f 0x76 0x08 0xd5 == dc ivac
