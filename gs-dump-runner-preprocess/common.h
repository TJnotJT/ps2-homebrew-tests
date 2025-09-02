#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>


#ifndef __TAMTYPES_H__
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;
#endif

#ifndef __COMMON_H__
#define __COMMON_H__

// GS dump tags
#define GS_DUMP_TRANSFER 0
#define GS_DUMP_VSYNC 1
#define GS_DUMP_FIFO 2
#define GS_DUMP_REGISTERS 3
#define GS_DUMP_FREEZE 4
#define GS_DUMP_UNKNOWN 5


// Size of different things in bytes
#define GS_PRIV_REG_IN_SIZE 8192
#define GS_MEMORY_SIZE 0x400000
#define MAX_DMA_SIZE 0x100000


// typedef union {
//   u8 U8[16];
//   u16 U16[8];
//   u32 U32[4];
//   u64 U64[2];
//   int8_t S8[16];
//   int16_t S16[8];
//   int32_t S32[4];
//   int64_t S64[2];
//   float F32[4];
//   double F64[2];
// } qword_t;

// static_assert(sizeof(qword_t) == 16, "Invalid size");


typedef struct {
  u32 tag;
  union
  {
    u32 path; // For tag == GS_DUMP_TRANSFER
    u32 field; // For tag == GS_DUMP_VSYNC
  };
  u32 size;
  u32 command_data_offset; // Offset in command_data. Must always be quad word aligned.
} command_t;


typedef struct 
{
  u64 PMODE;
  u64 SMODE1;
  u64 SMODE2;
  u64 SRFSH;
  u64 SYNCH1;
  u64 SYNCH2;
  u64 SYNCV;
  struct
  {
    u64 DISPFB;
    u64 DISPLAY;
  } DISP[2];
  u64 EXTBUF;
  u64 EXTDATA;
  u64 EXTWRITE;
  u64 BGCOLOR;
  u64 CSR;
  u64 IMR;
  u64 BUSDIR;
  u64 SIGLBLID;
} priv_regs_out_t; // GS privileged registers in struct format


typedef struct
{
  union
  {
    struct
    {
      u64 PMODE;
      u64 _pad1;
      u64 SMODE1;
      u64 _pad2;
      u64 SMODE2;
      u64 _pad3;
      u64 SRFSH;
      u64 _pad4;
      u64 SYNCH1;
      u64 _pad5;
      u64 SYNCH2;
      u64 _pad6;
      u64 SYNCV;
      u64 _pad7;
      struct
      {
        u64 DISPFB;
        u64 _pad1;
        u64 DISPLAY;
        u64 _pad2;
      } DISP[2];
      u64 EXTBUF;
      u64 _pad8;
      u64 EXTDATA;
      u64 _pad9;
      u64 EXTWRITE;
      u64 _pad10;
      u64 BGCOLOR;
      u64 _pad11;
    };

    u8 _pad12[0x1000];
  };

  union
  {
    struct
    {
      u64 CSR;
      u64 _pad13;
      u64 IMR;
      u64 _pad14;
      u64 _unk1[4];
      u64 BUSDIR;
      u64 _pad15;
      u64 _unk2[6];
      u64 SIGLBLID;
      u64 _pad16;
    };

    u8 _pad17[0x1000];
  };
} priv_reg_in_t; // GS privileged registers in raw format

static_assert(sizeof(priv_reg_in_t) == GS_PRIV_REG_IN_SIZE, "Invalid size");


typedef struct
{
  u64 PRIM;
  u64 PRMODECONT;
  u64 TEXCLUT;
  u64 SCANMSK;
  u64 TEXA;
  u64 FOGCOL;
  u64 DIMX;
  u64 DTHE;
  u64 COLCLAMP;
  u64 PABE;
  u64 BITBLTBUF;
  u64 TRXDIR;
  u64 TRXPOS;
  u64 TRXREG_1;
  u64 TRXREG_2;
  u64 XYOFFSET_1;
  u64 TEX0_1;
  u64 TEX1_1;
  u64 CLAMP_1;
  u64 MIPTBP1_1;
  u64 MIPTBP2_1;
  u64 SCISSOR_1;
  u64 ALPHA_1;
  u64 TEST_1;
  u64 FBA_1;
  u64 FRAME_1;
  u64 ZBUF_1;
  u64 XYOFFSET_2;
  u64 TEX0_2;
  u64 TEX1_2;
  u64 CLAMP_2;
  u64 MIPTBP1_2;
  u64 MIPTBP2_2;
  u64 SCISSOR_2;
  u64 ALPHA_2;
  u64 TEST_2;
  u64 FBA_2;
  u64 FRAME_2;
  u64 ZBUF_2;
  u64 RGBAQ;
  u64 ST;
  u64 UV;
  u64 FOG;
  u64 XYZ2;
} general_regs_t;

typedef struct {
  u64 RC : 3;
  u64 LC : 7;
  u64 T1248 : 2;
  u64 SLCK : 1;
  u64 CMOD : 2;
  u64 EX : 1;
  u64 PRST : 1;
  u64 SINT : 1;
  u64 XPCK : 1;
  u64 PCK2 : 2;
  u64 SPML : 4;
  u64 GCONT : 1;
  u64 PHS : 1;
  u64 PVS : 1;
  u64 PEHS : 1;
  u64 PEVS : 1;
  u64 CLKSEL : 2;
  u64 NVCK : 1;
  u64 SLCK2 : 1;
  u64 VCKSEL : 2;
  u64 VHP : 2;
} SMODE1_t;

#endif