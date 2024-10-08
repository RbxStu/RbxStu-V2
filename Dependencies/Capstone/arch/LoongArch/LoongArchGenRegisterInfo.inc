#ifdef GET_REGINFO_ENUM
#undef GET_REGINFO_ENUM

enum {
  LoongArch_NoRegister,
  LoongArch_F0 = 1,
  LoongArch_F1 = 2,
  LoongArch_F2 = 3,
  LoongArch_F3 = 4,
  LoongArch_F4 = 5,
  LoongArch_F5 = 6,
  LoongArch_F6 = 7,
  LoongArch_F7 = 8,
  LoongArch_F8 = 9,
  LoongArch_F9 = 10,
  LoongArch_F10 = 11,
  LoongArch_F11 = 12,
  LoongArch_F12 = 13,
  LoongArch_F13 = 14,
  LoongArch_F14 = 15,
  LoongArch_F15 = 16,
  LoongArch_F16 = 17,
  LoongArch_F17 = 18,
  LoongArch_F18 = 19,
  LoongArch_F19 = 20,
  LoongArch_F20 = 21,
  LoongArch_F21 = 22,
  LoongArch_F22 = 23,
  LoongArch_F23 = 24,
  LoongArch_F24 = 25,
  LoongArch_F25 = 26,
  LoongArch_F26 = 27,
  LoongArch_F27 = 28,
  LoongArch_F28 = 29,
  LoongArch_F29 = 30,
  LoongArch_F30 = 31,
  LoongArch_F31 = 32,
  LoongArch_FCC0 = 33,
  LoongArch_FCC1 = 34,
  LoongArch_FCC2 = 35,
  LoongArch_FCC3 = 36,
  LoongArch_FCC4 = 37,
  LoongArch_FCC5 = 38,
  LoongArch_FCC6 = 39,
  LoongArch_FCC7 = 40,
  LoongArch_FCSR0 = 41,
  LoongArch_FCSR1 = 42,
  LoongArch_FCSR2 = 43,
  LoongArch_FCSR3 = 44,
  LoongArch_R0 = 45,
  LoongArch_R1 = 46,
  LoongArch_R2 = 47,
  LoongArch_R3 = 48,
  LoongArch_R4 = 49,
  LoongArch_R5 = 50,
  LoongArch_R6 = 51,
  LoongArch_R7 = 52,
  LoongArch_R8 = 53,
  LoongArch_R9 = 54,
  LoongArch_R10 = 55,
  LoongArch_R11 = 56,
  LoongArch_R12 = 57,
  LoongArch_R13 = 58,
  LoongArch_R14 = 59,
  LoongArch_R15 = 60,
  LoongArch_R16 = 61,
  LoongArch_R17 = 62,
  LoongArch_R18 = 63,
  LoongArch_R19 = 64,
  LoongArch_R20 = 65,
  LoongArch_R21 = 66,
  LoongArch_R22 = 67,
  LoongArch_R23 = 68,
  LoongArch_R24 = 69,
  LoongArch_R25 = 70,
  LoongArch_R26 = 71,
  LoongArch_R27 = 72,
  LoongArch_R28 = 73,
  LoongArch_R29 = 74,
  LoongArch_R30 = 75,
  LoongArch_R31 = 76,
  LoongArch_SCR0 = 77,
  LoongArch_SCR1 = 78,
  LoongArch_SCR2 = 79,
  LoongArch_SCR3 = 80,
  LoongArch_VR0 = 81,
  LoongArch_VR1 = 82,
  LoongArch_VR2 = 83,
  LoongArch_VR3 = 84,
  LoongArch_VR4 = 85,
  LoongArch_VR5 = 86,
  LoongArch_VR6 = 87,
  LoongArch_VR7 = 88,
  LoongArch_VR8 = 89,
  LoongArch_VR9 = 90,
  LoongArch_VR10 = 91,
  LoongArch_VR11 = 92,
  LoongArch_VR12 = 93,
  LoongArch_VR13 = 94,
  LoongArch_VR14 = 95,
  LoongArch_VR15 = 96,
  LoongArch_VR16 = 97,
  LoongArch_VR17 = 98,
  LoongArch_VR18 = 99,
  LoongArch_VR19 = 100,
  LoongArch_VR20 = 101,
  LoongArch_VR21 = 102,
  LoongArch_VR22 = 103,
  LoongArch_VR23 = 104,
  LoongArch_VR24 = 105,
  LoongArch_VR25 = 106,
  LoongArch_VR26 = 107,
  LoongArch_VR27 = 108,
  LoongArch_VR28 = 109,
  LoongArch_VR29 = 110,
  LoongArch_VR30 = 111,
  LoongArch_VR31 = 112,
  LoongArch_XR0 = 113,
  LoongArch_XR1 = 114,
  LoongArch_XR2 = 115,
  LoongArch_XR3 = 116,
  LoongArch_XR4 = 117,
  LoongArch_XR5 = 118,
  LoongArch_XR6 = 119,
  LoongArch_XR7 = 120,
  LoongArch_XR8 = 121,
  LoongArch_XR9 = 122,
  LoongArch_XR10 = 123,
  LoongArch_XR11 = 124,
  LoongArch_XR12 = 125,
  LoongArch_XR13 = 126,
  LoongArch_XR14 = 127,
  LoongArch_XR15 = 128,
  LoongArch_XR16 = 129,
  LoongArch_XR17 = 130,
  LoongArch_XR18 = 131,
  LoongArch_XR19 = 132,
  LoongArch_XR20 = 133,
  LoongArch_XR21 = 134,
  LoongArch_XR22 = 135,
  LoongArch_XR23 = 136,
  LoongArch_XR24 = 137,
  LoongArch_XR25 = 138,
  LoongArch_XR26 = 139,
  LoongArch_XR27 = 140,
  LoongArch_XR28 = 141,
  LoongArch_XR29 = 142,
  LoongArch_XR30 = 143,
  LoongArch_XR31 = 144,
  LoongArch_F0_64 = 145,
  LoongArch_F1_64 = 146,
  LoongArch_F2_64 = 147,
  LoongArch_F3_64 = 148,
  LoongArch_F4_64 = 149,
  LoongArch_F5_64 = 150,
  LoongArch_F6_64 = 151,
  LoongArch_F7_64 = 152,
  LoongArch_F8_64 = 153,
  LoongArch_F9_64 = 154,
  LoongArch_F10_64 = 155,
  LoongArch_F11_64 = 156,
  LoongArch_F12_64 = 157,
  LoongArch_F13_64 = 158,
  LoongArch_F14_64 = 159,
  LoongArch_F15_64 = 160,
  LoongArch_F16_64 = 161,
  LoongArch_F17_64 = 162,
  LoongArch_F18_64 = 163,
  LoongArch_F19_64 = 164,
  LoongArch_F20_64 = 165,
  LoongArch_F21_64 = 166,
  LoongArch_F22_64 = 167,
  LoongArch_F23_64 = 168,
  LoongArch_F24_64 = 169,
  LoongArch_F25_64 = 170,
  LoongArch_F26_64 = 171,
  LoongArch_F27_64 = 172,
  LoongArch_F28_64 = 173,
  LoongArch_F29_64 = 174,
  LoongArch_F30_64 = 175,
  LoongArch_F31_64 = 176,
  NUM_TARGET_REGS // 177
};

// Register classes

enum {
  LoongArch_FPR32RegClassID = 0,
  LoongArch_GPRRegClassID = 1,
  LoongArch_GPRTRegClassID = 2,
  LoongArch_CFRRegClassID = 3,
  LoongArch_FCSRRegClassID = 4,
  LoongArch_SCRRegClassID = 5,
  LoongArch_FPR64RegClassID = 6,
  LoongArch_LSX128RegClassID = 7,
  LoongArch_LASX256RegClassID = 8,

};

// Register alternate name indices

enum {
  LoongArch_NoRegAltName,	// 0
  LoongArch_RegAliasName,	// 1
  NUM_TARGET_REG_ALT_NAMES = 2
};

// Subregister indices

enum {
  LoongArch_NoSubRegister,
  LoongArch_sub_32,	// 1
  LoongArch_sub_64,	// 2
  LoongArch_sub_128,	// 3
  LoongArch_NUM_TARGET_SUBREGS
};
#endif // GET_REGINFO_ENUM

/* Capstone Disassembly Engine, https://www.capstone-engine.org */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2022, */
/*    Rot127 <unisono@quyllur.org> 2022-2024 */
/* Automatically generated file by Capstone's LLVM TableGen Disassembler Backend. */

/* LLVM-commit: <commit> */
/* LLVM-tag: <tag> */

/* Do not edit. */

/* Capstone's LLVM TableGen Backends: */
/* https://github.com/capstone-engine/llvm-capstone */

#ifdef GET_REGINFO_MC_DESC
#undef GET_REGINFO_MC_DESC

static const MCPhysReg LoongArchRegDiffLists[] = {
  /* 0 */ -32, 64, -144, 0,
  /* 4 */ 144, -64, 32, 0,
};

static const uint16_t LoongArchSubRegIdxLists[] = {
  /* 0 */ 3, 2, 1, 0,
};

static const MCRegisterDesc LoongArchRegDesc[] = { // Descriptors
  { 3, 0, 0, 0, 0, 0 },
  { 47, 3, 4, 3, 12288, 2 },
  { 116, 3, 4, 3, 12289, 2 },
  { 171, 3, 4, 3, 12290, 2 },
  { 226, 3, 4, 3, 12291, 2 },
  { 495, 3, 4, 3, 12292, 2 },
  { 539, 3, 4, 3, 12293, 2 },
  { 583, 3, 4, 3, 12294, 2 },
  { 627, 3, 4, 3, 12295, 2 },
  { 666, 3, 4, 3, 12296, 2 },
  { 705, 3, 4, 3, 12297, 2 },
  { 0, 3, 4, 3, 12298, 2 },
  { 69, 3, 4, 3, 12299, 2 },
  { 138, 3, 4, 3, 12300, 2 },
  { 193, 3, 4, 3, 12301, 2 },
  { 248, 3, 4, 3, 12302, 2 },
  { 506, 3, 4, 3, 12303, 2 },
  { 550, 3, 4, 3, 12304, 2 },
  { 594, 3, 4, 3, 12305, 2 },
  { 638, 3, 4, 3, 12306, 2 },
  { 677, 3, 4, 3, 12307, 2 },
  { 14, 3, 4, 3, 12308, 2 },
  { 83, 3, 4, 3, 12309, 2 },
  { 152, 3, 4, 3, 12310, 2 },
  { 207, 3, 4, 3, 12311, 2 },
  { 262, 3, 4, 3, 12312, 2 },
  { 520, 3, 4, 3, 12313, 2 },
  { 564, 3, 4, 3, 12314, 2 },
  { 608, 3, 4, 3, 12315, 2 },
  { 652, 3, 4, 3, 12316, 2 },
  { 691, 3, 4, 3, 12317, 2 },
  { 28, 3, 4, 3, 12318, 2 },
  { 97, 3, 4, 3, 12319, 2 },
  { 42, 3, 3, 3, 12320, 2 },
  { 111, 3, 3, 3, 12321, 2 },
  { 166, 3, 3, 3, 12322, 2 },
  { 221, 3, 3, 3, 12323, 2 },
  { 490, 3, 3, 3, 12324, 2 },
  { 534, 3, 3, 3, 12325, 2 },
  { 578, 3, 3, 3, 12326, 2 },
  { 622, 3, 3, 3, 12327, 2 },
  { 55, 3, 3, 3, 12328, 2 },
  { 124, 3, 3, 3, 12329, 2 },
  { 179, 3, 3, 3, 12330, 2 },
  { 234, 3, 3, 3, 12331, 2 },
  { 52, 3, 3, 3, 12332, 2 },
  { 121, 3, 3, 3, 12333, 2 },
  { 176, 3, 3, 3, 12334, 2 },
  { 231, 3, 3, 3, 12335, 2 },
  { 499, 3, 3, 3, 12336, 2 },
  { 543, 3, 3, 3, 12337, 2 },
  { 587, 3, 3, 3, 12338, 2 },
  { 631, 3, 3, 3, 12339, 2 },
  { 670, 3, 3, 3, 12340, 2 },
  { 709, 3, 3, 3, 12341, 2 },
  { 5, 3, 3, 3, 12342, 2 },
  { 74, 3, 3, 3, 12343, 2 },
  { 143, 3, 3, 3, 12344, 2 },
  { 198, 3, 3, 3, 12345, 2 },
  { 253, 3, 3, 3, 12346, 2 },
  { 511, 3, 3, 3, 12347, 2 },
  { 555, 3, 3, 3, 12348, 2 },
  { 599, 3, 3, 3, 12349, 2 },
  { 643, 3, 3, 3, 12350, 2 },
  { 682, 3, 3, 3, 12351, 2 },
  { 19, 3, 3, 3, 12352, 2 },
  { 88, 3, 3, 3, 12353, 2 },
  { 157, 3, 3, 3, 12354, 2 },
  { 212, 3, 3, 3, 12355, 2 },
  { 267, 3, 3, 3, 12356, 2 },
  { 525, 3, 3, 3, 12357, 2 },
  { 569, 3, 3, 3, 12358, 2 },
  { 613, 3, 3, 3, 12359, 2 },
  { 657, 3, 3, 3, 12360, 2 },
  { 696, 3, 3, 3, 12361, 2 },
  { 33, 3, 3, 3, 12362, 2 },
  { 102, 3, 3, 3, 12363, 2 },
  { 50, 3, 3, 3, 12364, 2 },
  { 119, 3, 3, 3, 12365, 2 },
  { 174, 3, 3, 3, 12366, 2 },
  { 229, 3, 3, 3, 12367, 2 },
  { 61, 1, 6, 1, 12288, 0 },
  { 130, 1, 6, 1, 12289, 0 },
  { 185, 1, 6, 1, 12290, 0 },
  { 240, 1, 6, 1, 12291, 0 },
  { 498, 1, 6, 1, 12292, 0 },
  { 542, 1, 6, 1, 12293, 0 },
  { 586, 1, 6, 1, 12294, 0 },
  { 630, 1, 6, 1, 12295, 0 },
  { 669, 1, 6, 1, 12296, 0 },
  { 708, 1, 6, 1, 12297, 0 },
  { 4, 1, 6, 1, 12298, 0 },
  { 73, 1, 6, 1, 12299, 0 },
  { 142, 1, 6, 1, 12300, 0 },
  { 197, 1, 6, 1, 12301, 0 },
  { 252, 1, 6, 1, 12302, 0 },
  { 510, 1, 6, 1, 12303, 0 },
  { 554, 1, 6, 1, 12304, 0 },
  { 598, 1, 6, 1, 12305, 0 },
  { 642, 1, 6, 1, 12306, 0 },
  { 681, 1, 6, 1, 12307, 0 },
  { 18, 1, 6, 1, 12308, 0 },
  { 87, 1, 6, 1, 12309, 0 },
  { 156, 1, 6, 1, 12310, 0 },
  { 211, 1, 6, 1, 12311, 0 },
  { 266, 1, 6, 1, 12312, 0 },
  { 524, 1, 6, 1, 12313, 0 },
  { 568, 1, 6, 1, 12314, 0 },
  { 612, 1, 6, 1, 12315, 0 },
  { 656, 1, 6, 1, 12316, 0 },
  { 695, 1, 6, 1, 12317, 0 },
  { 32, 1, 6, 1, 12318, 0 },
  { 101, 1, 6, 1, 12319, 0 },
  { 65, 0, 3, 0, 12288, 0 },
  { 134, 0, 3, 0, 12289, 0 },
  { 189, 0, 3, 0, 12290, 0 },
  { 244, 0, 3, 0, 12291, 0 },
  { 502, 0, 3, 0, 12292, 0 },
  { 546, 0, 3, 0, 12293, 0 },
  { 590, 0, 3, 0, 12294, 0 },
  { 634, 0, 3, 0, 12295, 0 },
  { 673, 0, 3, 0, 12296, 0 },
  { 712, 0, 3, 0, 12297, 0 },
  { 9, 0, 3, 0, 12298, 0 },
  { 78, 0, 3, 0, 12299, 0 },
  { 147, 0, 3, 0, 12300, 0 },
  { 202, 0, 3, 0, 12301, 0 },
  { 257, 0, 3, 0, 12302, 0 },
  { 515, 0, 3, 0, 12303, 0 },
  { 559, 0, 3, 0, 12304, 0 },
  { 603, 0, 3, 0, 12305, 0 },
  { 647, 0, 3, 0, 12306, 0 },
  { 686, 0, 3, 0, 12307, 0 },
  { 23, 0, 3, 0, 12308, 0 },
  { 92, 0, 3, 0, 12309, 0 },
  { 161, 0, 3, 0, 12310, 0 },
  { 216, 0, 3, 0, 12311, 0 },
  { 271, 0, 3, 0, 12312, 0 },
  { 529, 0, 3, 0, 12313, 0 },
  { 573, 0, 3, 0, 12314, 0 },
  { 617, 0, 3, 0, 12315, 0 },
  { 661, 0, 3, 0, 12316, 0 },
  { 700, 0, 3, 0, 12317, 0 },
  { 37, 0, 3, 0, 12318, 0 },
  { 106, 0, 3, 0, 12319, 0 },
  { 297, 2, 5, 2, 12288, 0 },
  { 324, 2, 5, 2, 12289, 0 },
  { 344, 2, 5, 2, 12290, 0 },
  { 364, 2, 5, 2, 12291, 0 },
  { 384, 2, 5, 2, 12292, 0 },
  { 404, 2, 5, 2, 12293, 0 },
  { 424, 2, 5, 2, 12294, 0 },
  { 444, 2, 5, 2, 12295, 0 },
  { 464, 2, 5, 2, 12296, 0 },
  { 484, 2, 5, 2, 12297, 0 },
  { 276, 2, 5, 2, 12298, 0 },
  { 303, 2, 5, 2, 12299, 0 },
  { 330, 2, 5, 2, 12300, 0 },
  { 350, 2, 5, 2, 12301, 0 },
  { 370, 2, 5, 2, 12302, 0 },
  { 390, 2, 5, 2, 12303, 0 },
  { 410, 2, 5, 2, 12304, 0 },
  { 430, 2, 5, 2, 12305, 0 },
  { 450, 2, 5, 2, 12306, 0 },
  { 470, 2, 5, 2, 12307, 0 },
  { 283, 2, 5, 2, 12308, 0 },
  { 310, 2, 5, 2, 12309, 0 },
  { 337, 2, 5, 2, 12310, 0 },
  { 357, 2, 5, 2, 12311, 0 },
  { 377, 2, 5, 2, 12312, 0 },
  { 397, 2, 5, 2, 12313, 0 },
  { 417, 2, 5, 2, 12314, 0 },
  { 437, 2, 5, 2, 12315, 0 },
  { 457, 2, 5, 2, 12316, 0 },
  { 477, 2, 5, 2, 12317, 0 },
  { 290, 2, 5, 2, 12318, 0 },
  { 317, 2, 5, 2, 12319, 0 },
};

  // FPR32 Register Class...
  static const MCPhysReg FPR32[] = {
    LoongArch_F0, LoongArch_F1, LoongArch_F2, LoongArch_F3, LoongArch_F4, LoongArch_F5, LoongArch_F6, LoongArch_F7, LoongArch_F8, LoongArch_F9, LoongArch_F10, LoongArch_F11, LoongArch_F12, LoongArch_F13, LoongArch_F14, LoongArch_F15, LoongArch_F16, LoongArch_F17, LoongArch_F18, LoongArch_F19, LoongArch_F20, LoongArch_F21, LoongArch_F22, LoongArch_F23, LoongArch_F24, LoongArch_F25, LoongArch_F26, LoongArch_F27, LoongArch_F28, LoongArch_F29, LoongArch_F30, LoongArch_F31, 
  };

  // FPR32 Bit set.
  static const uint8_t FPR32Bits[] = {
    0xfe, 0xff, 0xff, 0xff, 0x01, 
  };

  // GPR Register Class...
  static const MCPhysReg GPR[] = {
    LoongArch_R4, LoongArch_R5, LoongArch_R6, LoongArch_R7, LoongArch_R8, LoongArch_R9, LoongArch_R10, LoongArch_R11, LoongArch_R12, LoongArch_R13, LoongArch_R14, LoongArch_R15, LoongArch_R16, LoongArch_R17, LoongArch_R18, LoongArch_R19, LoongArch_R20, LoongArch_R22, LoongArch_R23, LoongArch_R24, LoongArch_R25, LoongArch_R26, LoongArch_R27, LoongArch_R28, LoongArch_R29, LoongArch_R30, LoongArch_R31, LoongArch_R0, LoongArch_R1, LoongArch_R2, LoongArch_R3, LoongArch_R21, 
  };

  // GPR Bit set.
  static const uint8_t GPRBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // GPRT Register Class...
  static const MCPhysReg GPRT[] = {
    LoongArch_R4, LoongArch_R5, LoongArch_R6, LoongArch_R7, LoongArch_R8, LoongArch_R9, LoongArch_R10, LoongArch_R11, LoongArch_R12, LoongArch_R13, LoongArch_R14, LoongArch_R15, LoongArch_R16, LoongArch_R17, LoongArch_R18, LoongArch_R19, LoongArch_R20, 
  };

  // GPRT Bit set.
  static const uint8_t GPRTBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x03, 
  };

  // CFR Register Class...
  static const MCPhysReg CFR[] = {
    LoongArch_FCC0, LoongArch_FCC1, LoongArch_FCC2, LoongArch_FCC3, LoongArch_FCC4, LoongArch_FCC5, LoongArch_FCC6, LoongArch_FCC7, 
  };

  // CFR Bit set.
  static const uint8_t CFRBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xfe, 0x01, 
  };

  // FCSR Register Class...
  static const MCPhysReg FCSR[] = {
    LoongArch_FCSR0, LoongArch_FCSR1, LoongArch_FCSR2, LoongArch_FCSR3, 
  };

  // FCSR Bit set.
  static const uint8_t FCSRBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 
  };

  // SCR Register Class...
  static const MCPhysReg SCR[] = {
    LoongArch_SCR0, LoongArch_SCR1, LoongArch_SCR2, LoongArch_SCR3, 
  };

  // SCR Bit set.
  static const uint8_t SCRBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 
  };

  // FPR64 Register Class...
  static const MCPhysReg FPR64[] = {
    LoongArch_F0_64, LoongArch_F1_64, LoongArch_F2_64, LoongArch_F3_64, LoongArch_F4_64, LoongArch_F5_64, LoongArch_F6_64, LoongArch_F7_64, LoongArch_F8_64, LoongArch_F9_64, LoongArch_F10_64, LoongArch_F11_64, LoongArch_F12_64, LoongArch_F13_64, LoongArch_F14_64, LoongArch_F15_64, LoongArch_F16_64, LoongArch_F17_64, LoongArch_F18_64, LoongArch_F19_64, LoongArch_F20_64, LoongArch_F21_64, LoongArch_F22_64, LoongArch_F23_64, LoongArch_F24_64, LoongArch_F25_64, LoongArch_F26_64, LoongArch_F27_64, LoongArch_F28_64, LoongArch_F29_64, LoongArch_F30_64, LoongArch_F31_64, 
  };

  // FPR64 Bit set.
  static const uint8_t FPR64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x01, 
  };

  // LSX128 Register Class...
  static const MCPhysReg LSX128[] = {
    LoongArch_VR0, LoongArch_VR1, LoongArch_VR2, LoongArch_VR3, LoongArch_VR4, LoongArch_VR5, LoongArch_VR6, LoongArch_VR7, LoongArch_VR8, LoongArch_VR9, LoongArch_VR10, LoongArch_VR11, LoongArch_VR12, LoongArch_VR13, LoongArch_VR14, LoongArch_VR15, LoongArch_VR16, LoongArch_VR17, LoongArch_VR18, LoongArch_VR19, LoongArch_VR20, LoongArch_VR21, LoongArch_VR22, LoongArch_VR23, LoongArch_VR24, LoongArch_VR25, LoongArch_VR26, LoongArch_VR27, LoongArch_VR28, LoongArch_VR29, LoongArch_VR30, LoongArch_VR31, 
  };

  // LSX128 Bit set.
  static const uint8_t LSX128Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x01, 
  };

  // LASX256 Register Class...
  static const MCPhysReg LASX256[] = {
    LoongArch_XR0, LoongArch_XR1, LoongArch_XR2, LoongArch_XR3, LoongArch_XR4, LoongArch_XR5, LoongArch_XR6, LoongArch_XR7, LoongArch_XR8, LoongArch_XR9, LoongArch_XR10, LoongArch_XR11, LoongArch_XR12, LoongArch_XR13, LoongArch_XR14, LoongArch_XR15, LoongArch_XR16, LoongArch_XR17, LoongArch_XR18, LoongArch_XR19, LoongArch_XR20, LoongArch_XR21, LoongArch_XR22, LoongArch_XR23, LoongArch_XR24, LoongArch_XR25, LoongArch_XR26, LoongArch_XR27, LoongArch_XR28, LoongArch_XR29, LoongArch_XR30, LoongArch_XR31, 
  };

  // LASX256 Bit set.
  static const uint8_t LASX256Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x01, 
  };

static const MCRegisterClass LoongArchMCRegisterClasses[] = {
  { FPR32, FPR32Bits, sizeof(FPR32Bits) },
  { GPR, GPRBits, sizeof(GPRBits) },
  { GPRT, GPRTBits, sizeof(GPRTBits) },
  { CFR, CFRBits, sizeof(CFRBits) },
  { FCSR, FCSRBits, sizeof(FCSRBits) },
  { SCR, SCRBits, sizeof(SCRBits) },
  { FPR64, FPR64Bits, sizeof(FPR64Bits) },
  { LSX128, LSX128Bits, sizeof(LSX128Bits) },
  { LASX256, LASX256Bits, sizeof(LASX256Bits) },
};

static const uint16_t LoongArchRegEncodingTable[] = {
  0,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23,
  24,
  25,
  26,
  27,
  28,
  29,
  30,
  31,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  0,
  1,
  2,
  3,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23,
  24,
  25,
  26,
  27,
  28,
  29,
  30,
  31,
  0,
  1,
  2,
  3,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23,
  24,
  25,
  26,
  27,
  28,
  29,
  30,
  31,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23,
  24,
  25,
  26,
  27,
  28,
  29,
  30,
  31,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23,
  24,
  25,
  26,
  27,
  28,
  29,
  30,
  31,
};
#endif // GET_REGINFO_MC_DESC



