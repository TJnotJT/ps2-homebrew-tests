#pragma once

#include <gs_psm.h>
#include <assert.h>
#include <tamtypes.h>

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
