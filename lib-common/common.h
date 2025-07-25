#pragma once

#include <gs_psm.h>
#include <assert.h>
#include <tamtypes.h>

#define PAGE_BLOCKS 32
#define BLOCK_COLUMNS 4
#define COLUMN_WORDS 32
#define PAGE_COLUMNS (PAGE_BLOCKS * BLOCK_COLUMNS)
#define PAGE_WORDS (PAGE_COLUMNS * COLUMN_WORDS)
#define BLOCK_WORDS (BLOCK_COLUMNS * COLUMN_WORDS)



static inline u32 gs_psm_bpp(u32 psm)
{
  switch (psm)
  {
    case GS_PSM_32: return 32;
    case GS_PSM_24: return 32;
    case GS_PSM_16: return 16;
    case GS_PSM_16S: return 16;
    case GS_PSM_PS24: return 32;
    case GS_PSM_8: return 8;
    case GS_PSM_4: return 4;
    case GS_PSM_8H: return 32;
    case GS_PSM_4HL: return 32;
    case GS_PSM_4HH: return 32;
    case GS_PSMZ_32: return 32;
    case GS_PSMZ_24: return 32;
    case GS_PSMZ_16: return 16;
    case GS_PSMZ_16S: return 16;
    default: assert(0 && "Unsupported PSM in gs_psm_bpp");
  }
  return 0; // Should never reach here, but added to avoid compiler warnings
}

typedef union uv_t {
	struct {
		u16 u;
		u16 v;
		u32 pad;
	};
	u32 uv;
} uv_t __attribute__((aligned(16)));

static inline u32 prim_verts(u32 prim_type)
{
	switch (prim_type)
	{
    case PRIM_POINT: return 1;
    case PRIM_LINE: return 2;
    case PRIM_LINE_STRIP: return 2;
		case PRIM_TRIANGLE: return 3;
    case PRIM_TRIANGLE_STRIP: return 3;
    case PRIM_TRIANGLE_FAN: return 3;
		case PRIM_SPRITE: return 2;
		default: assert(0 && "Unsupported primitive type");
	}
	return 0;
}