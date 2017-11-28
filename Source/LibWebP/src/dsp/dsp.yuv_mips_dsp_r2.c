// Copyright 2014 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// MIPS DSPr2 version of YUV to RGB upsampling functions.
//
// Author(s):  Branimir Vasic (branimir.vasic@imgtec.com)
//             Djordje Pesut  (djordje.pesut@imgtec.com)

#include "./dsp.h"

#if defined(WEBP_USE_MIPS_DSP_R2)

#include "./yuv.h"

//------------------------------------------------------------------------------
// simple point-sampling

#define ROW_FUNC_PART_1()                                                      \
  "lbu              %[temp3],   0(%[v])                         \n\t"          \
  "lbu              %[temp4],   0(%[u])                         \n\t"          \
  "lbu              %[temp0],   0(%[y])                         \n\t"          \
  "mul              %[temp1],   %[t_con_1],     %[temp3]        \n\t"          \
  "mul              %[temp3],   %[t_con_2],     %[temp3]        \n\t"          \
  "mul              %[temp2],   %[t_con_3],     %[temp4]        \n\t"          \
  "mul              %[temp4],   %[t_con_4],     %[temp4]        \n\t"          \
  "mul              %[temp0],   %[t_con_5],     %[temp0]        \n\t"          \
  "addu             %[temp1],   %[temp1],       %[t_con_6]      \n\t"          \
  "subu             %[temp3],   %[temp3],       %[t_con_7]      \n\t"          \
  "addu             %[temp2],   %[temp2],       %[temp3]        \n\t"          \
  "addu             %[temp4],   %[temp4],       %[t_con_8]      \n\t"          \

#define ROW_FUNC_PART_2(r, g, b, K)                                            \
  "addu             %[temp5],   %[temp0],       %[temp1]        \n\t"          \
  "subu             %[temp6],   %[temp0],       %[temp2]        \n\t"          \
  "addu             %[temp7],   %[temp0],       %[temp4]        \n\t"          \
".if "#K"                                                       \n\t"          \
  "lbu              %[temp0],   1(%[y])                         \n\t"          \
".endif                                                         \n\t"          \
  "shll_s.w         %[temp5],   %[temp5],       9               \n\t"          \
  "shll_s.w         %[temp6],   %[temp6],       9               \n\t"          \
".if "#K"                                                       \n\t"          \
  "mul              %[temp0],   %[t_con_5],     %[temp0]        \n\t"          \
".endif                                                         \n\t"          \
  "shll_s.w         %[temp7],   %[temp7],       9               \n\t"          \
  "precrqu_s.qb.ph  %[temp5],   %[temp5],       $zero           \n\t"          \
  "precrqu_s.qb.ph  %[temp6],   %[temp6],       $zero           \n\t"          \
  "precrqu_s.qb.ph  %[temp7],   %[temp7],       $zero           \n\t"          \
  "srl              %[temp5],   %[temp5],       24              \n\t"          \
  "srl              %[temp6],   %[temp6],       24              \n\t"          \
  "srl              %[temp7],   %[temp7],       24              \n\t"          \
  "sb               %[temp5],   "#r"(%[dst])                    \n\t"          \
  "sb               %[temp6],   "#g"(%[dst])                    \n\t"          \
  "sb               %[temp7],   "#b"(%[dst])                    \n\t"          \

#define ASM_CLOBBER_LIST()                                                     \
  : [temp0]"=&r"(temp0), [temp1]"=&r"(temp1), [temp2]"=&r"(temp2),             \
    [temp3]"=&r"(temp3), [temp4]"=&r"(temp4), [temp5]"=&r"(temp5),             \
    [temp6]"=&r"(temp6), [temp7]"=&r"(temp7)                                   \
  : [t_con_1]"r"(t_con_1), [t_con_2]"r"(t_con_2), [t_con_3]"r"(t_con_3),       \
    [t_con_4]"r"(t_con_4), [t_con_5]"r"(t_con_5), [t_con_6]"r"(t_con_6),       \
    [u]"r"(u), [v]"r"(v), [y]"r"(y), [dst]"r"(dst),                            \
    [t_con_7]"r"(t_con_7), [t_con_8]"r"(t_con_8)                               \
  : "memory", "hi", "lo"                                                       \

#define ROW_FUNC(FUNC_NAME, XSTEP, R, G, B, A)                                 \
static void FUNC_NAME(const uint8_t* y,                                        \
                      const uint8_t* u, const uint8_t* v,                      \
                      uint8_t* dst, int len) {                                 \
  int i;                                                                       \
  uint32_t temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;             \
  const int t_con_1 = kVToR;                                                   \
  const int t_con_2 = kVToG;                                                   \
  const int t_con_3 = kUToG;                                                   \
  const int t_con_4 = kUToB;                                                   \
  const int t_con_5 = kYScale;                                                 \
  const int t_con_6 = kRCst;                                                   \
  const int t_con_7 = kGCst;                                                   \
  const int t_con_8 = kBCst;                                                   \
  for (i = 0; i < (len >> 1); i++) {                                           \
    __asm__ volatile (                                                         \
      ROW_FUNC_PART_1()                                                        \
      ROW_FUNC_PART_2(R, G, B, 1)                                              \
      ROW_FUNC_PART_2(R + XSTEP, G + XSTEP, B + XSTEP, 0)                      \
      ASM_CLOBBER_LIST()                                                       \
    );                                                                         \
    if (A) dst[A] = dst[A + XSTEP] = 0xff;                                     \
    y += 2;                                                                    \
    ++u;                                                                       \
    ++v;                                                                       \
    dst += 2 * XSTEP;                                                          \
  }                                                                            \
  if (len & 1) {                                                               \
    __asm__ volatile (                                                         \
      ROW_FUNC_PART_1()                                                        \
      ROW_FUNC_PART_2(R, G, B, 0)                                              \
      ASM_CLOBBER_LIST()                                                       \
    );                                                                         \
    if (A) dst[A] = 0xff;                                                      \
  }                                                                            \
}

ROW_FUNC(YuvToRgbRow,      3, 0, 1, 2, 0)
ROW_FUNC(YuvToRgbaRow,     4, 0, 1, 2, 3)
ROW_FUNC(YuvToBgrRow,      3, 2, 1, 0, 0)
ROW_FUNC(YuvToBgraRow,     4, 2, 1, 0, 3)

#undef ROW_FUNC
#undef ASM_CLOBBER_LIST
#undef ROW_FUNC_PART_2
#undef ROW_FUNC_PART_1

#endif  // WEBP_USE_MIPS_DSP_R2

//------------------------------------------------------------------------------

extern void WebPInitSamplersMIPSdspR2(void);

WEBP_TSAN_IGNORE_FUNCTION void WebPInitSamplersMIPSdspR2(void) {
#if defined(WEBP_USE_MIPS_DSP_R2)
  WebPSamplers[MODE_RGB]  = YuvToRgbRow;
  WebPSamplers[MODE_RGBA] = YuvToRgbaRow;
  WebPSamplers[MODE_BGR]  = YuvToBgrRow;
  WebPSamplers[MODE_BGRA] = YuvToBgraRow;
#endif  // WEBP_USE_MIPS_DSP_R2
}
