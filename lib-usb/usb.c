
#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <tamtypes.h>
#include <string.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sifrpc.h>
#include <dirent.h> // mkdir()
#include <unistd.h> // rmdir()
#include <sbv_patches.h>

#include "usb.h"
#include "../lib-bmp/bmp.h"
#include "../lib-common/common.h"

extern unsigned int size_bdm_irx;
extern unsigned int size_bdmfs_fatfs_irx;
extern unsigned int size_usbd_irx;
extern unsigned int size_usbmass_bd_irx;

extern unsigned char bdm_irx[];
extern unsigned char bdmfs_fatfs_irx[];
extern unsigned char usbd_irx[];
extern unsigned char usbmass_bd_irx[];

void busy_wait(u32 loops) {
	for (int i = 0; i < loops; i++)
		__asm volatile ("nop");
}

int wait_usb_ready()
{
	int retries = 10;
	while (retries > 0) {
		if (mkdir("mass:tmp", 0777) == 0)
		{
			rmdir("mass:tmp");
			return 0;
		}
		busy_wait(10 * 1000 * 1000);
		retries--;
	}
	return -1;
}

void reset_iop()
{
	SifInitRpc(0);
	// Reset IOP
	while (!SifIopReset("", 0x0))
		;
	while (!SifIopSync())
		;
	SifInitRpc(0);

	sbv_patch_enable_lmb();
}

void load_irx_usb()
{
	int bdm_irx_id = SifExecModuleBuffer(&bdm_irx, size_bdm_irx, 0, NULL, NULL);
	printf("BDM ID: %d\n", bdm_irx_id);

	int bdmfs_fatfs_irx_id = SifExecModuleBuffer(&bdmfs_fatfs_irx, size_bdmfs_fatfs_irx, 0, NULL, NULL);
	printf("BDMFS FATFS ID: %d\n", bdmfs_fatfs_irx_id);

	int usbd_irx_id = SifExecModuleBuffer(&usbd_irx, size_usbd_irx, 0, NULL, NULL);
	printf("USBD ID is %d\n", usbd_irx_id);

	int usbmass_irx_id = SifExecModuleBuffer(&usbmass_bd_irx, size_usbmass_bd_irx, 0, NULL, NULL);
	printf("USB Mass ID is %d\n", usbmass_irx_id);
}


#define COLOR_WAIT_USB 0x00FFFF
#define COLOR_WAIT_USB2 0x00A5FF
#define COLOR_FAIL 0x0000FF
#define COLOR_SUCCESS 0x00FF00
#define COLOR_DONE 0xFFFFFF

int write_bmp_to_usb(const char* filename, const u8* data, u32 width, u32 height, u32 psm, void (*set_debug_color)(u32 color))
{
	printf("Waiting for USB to be ready...\n");
	if (set_debug_color)
		set_debug_color(COLOR_WAIT_USB); // Clear screen to indicate start of USB operation

	FlushCache(0); // We read data with DMA so flush before writing to USB

	reset_iop();
	load_irx_usb();

	if (wait_usb_ready() != 0)
	{
		if (set_debug_color)
			set_debug_color(COLOR_FAIL); // Red background if USB not ready
		printf("USB not ready!\n");
		return -1;
	}

	printf("USB is ready, writing %s...\n", filename);

	if (set_debug_color)
		set_debug_color(COLOR_WAIT_USB2);

	if (write_bmp(filename, data, width, height, gs_psm_bpp(psm)) != 0)
	{
		if (set_debug_color)
			set_debug_color(COLOR_FAIL); // Red background if BMP write failed
		printf("Failed to write %s!\n", filename);
		return -1;
	}

	if (set_debug_color)
		set_debug_color(COLOR_SUCCESS);

	printf("Bitmap data written successfully to %s\n", filename);
	return 0;
}

#undef COLOR_WAIT_USB
#undef COLOR_WAIT_USB2
#undef COLOR_FAIL
#undef COLOR_SUCCESS
#undef COLOR_DONE