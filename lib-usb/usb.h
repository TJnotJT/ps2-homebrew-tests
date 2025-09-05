#pragma once

#include <tamtypes.h>
#include <gs_psm.h>

int wait_usb_ready();
void reset_iop();
void load_irx_usb();
int init_usb(void (*set_debug_color)(u32 color));
int write_bmp_to_usb(const char* filename, const u8* data, u32 width, u32 height, u32 psm, void (*set_debug_color)(u32 color));
