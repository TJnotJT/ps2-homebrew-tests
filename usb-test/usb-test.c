/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (c) 2005 Naomi Peori <naomi@peori.ca>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/

#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <tamtypes.h>
#include <math3d.h>
#include <string.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sifrpc.h>
#include <dirent.h> // mkdir()
#include <unistd.h> // rmdir()

#include <packet.h>

#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <gs_privileged.h>
#include <gs_gp.h>

#include <dma.h>

#include <graph.h>

#include <draw.h>
#include <draw3d.h>

#include <kernel.h>
#include <sio.h>
#include <debug.h>
#include <graph.h>
#include <draw.h>
#include <dma.h>
#include <gs_gp.h>
#include <gs_psm.h>

#include <sbv_patches.h>
#include <tamtypes.h>

#include "irx/bdm_irx.c"
#include "irx/bdmfs_fatfs_irx.c"
#include "irx/usbd_irx.c"
#include "irx/usbmass_bd_irx.c"

extern unsigned int size_bdm_irx;
extern unsigned int size_bdmfs_fatfs_irx;
extern unsigned int size_usbd_irx;
extern unsigned int size_usbmass_bd_irx;

extern unsigned char bdm_irx[];
extern unsigned char bdmfs_fatfs_irx[];
extern unsigned char usbd_irx[];
extern unsigned char usbmass_bd_irx[];

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 256
#define WINDOW_X (2048 - (FRAME_WIDTH / 2))
#define WINDOW_Y (2048 - (FRAME_HEIGHT / 2))
#define WAIT_COL 0x00ffff
#define FAIL_COL 0x0000ff
#define SUCCESS_COL 0x00ff00

framebuffer_t g_frame[2]; // Frame buffer for even/odd lines
zbuffer_t g_z; // Z buffer
texbuffer_t g_texbuf; // Texture buffer

qword_t buff[1024];

void busy_wait(u32 loops) {
	for (int i = 0; i < loops; i++)
		__asm volatile ("nop");
}

int graph_initialize_custom()
{
	int mode = graph_get_region();

	graph_set_mode(GRAPH_MODE_NONINTERLACED, mode, GRAPH_MODE_FRAME, GRAPH_ENABLE);

	graph_set_screen(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

	graph_set_bgcolor(0, 0, 0);

	graph_set_framebuffer(0, g_frame[0].address, g_frame[0].width, g_frame[0].psm, 0, 0);

	graph_set_output(1, 0, 1, 0, 1, 0xFF);

	return 0;
}

void init_gs()
{
	for (int i = 0; i < 1; i++)
	{
		g_frame[i].width = FRAME_WIDTH;
		g_frame[i].height = FRAME_HEIGHT;
		g_frame[i].mask = 0;
		g_frame[i].psm = GS_PSM_32;
		g_frame[i].address = graph_vram_allocate(g_frame[i].width, g_frame[i].height, g_frame[i].psm, GRAPH_ALIGN_PAGE);
	}

	// Enable the zbuffer.
	g_z.enable = DRAW_DISABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_GREATER_EQUAL;
	g_z.zsm = GS_ZBUF_24;
	g_z.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_24, GRAPH_ALIGN_PAGE);

	graph_initialize_custom();
}

void init_drawing_environment()
{
	qword_t *q = buff;

	q = draw_setup_environment(q, 0, &g_frame[0], &g_z);

	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);

	// Finish setting up the environment.
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, buff, q - buff, 0, 0);
	dma_wait_fast();
}

void my_draw_clear(u32 rgb)
{
	color_t bg_color;
	bg_color.r = (rgb >> 0) & 0xff;
	bg_color.g = (rgb >> 8) & 0xff;
	bg_color.b = (rgb >> 16) & 0xff;
	bg_color.a = 0x80;
	bg_color.q = 1.0f;

	qword_t* q = buff;

	PACK_GIFTAG(q, GIF_SET_TAG(7, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GIF_SET_PRIM(PRIM_TRIANGLE_STRIP, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
	q++;
	PACK_GIFTAG(q, bg_color.rgbaq, GIF_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(WINDOW_X << 4, WINDOW_X << 4, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, bg_color.rgbaq, GIF_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ((WINDOW_X + FRAME_WIDTH) << 4, WINDOW_Y << 4, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(WINDOW_X << 4, (WINDOW_Y + FRAME_HEIGHT) << 4, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ((WINDOW_X + FRAME_WIDTH) << 4, (WINDOW_Y + FRAME_HEIGHT) << 4, 0), GIF_REG_XYZ2);
	q++;

	dma_wait_fast();
	dma_channel_send_normal(DMA_CHANNEL_GIF, buff, q - buff, 0, 0);
}

int wait_usb_ready()
{
	int retries = 1000;
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
};

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	init_gs();

	init_drawing_environment();

	reset_iop();
	load_irx_usb();

	my_draw_clear(WAIT_COL);

	if (wait_usb_ready() == 0)
	{
		my_draw_clear(WAIT_COL);

		FILE* f;
		if ((f = fopen("mass:file", "wb")) != 0)
		{
			printf("Succeeded in opening file\n");
			my_draw_clear(SUCCESS_COL);
			const char* s = "This is a test";
			if (fwrite(s, 1, strlen(s), f) == strlen(s))
			{
				printf("Succeeded in writing data\n");
				fclose(f);
			}
			else
			{
				printf("Failed to write all data\n");
				my_draw_clear(FAIL_COL);
			}
		}
		else
		{
			printf("Failed to open file\n");
			my_draw_clear(FAIL_COL);
		}
	} else {
		// Could not detect the USB device
		my_draw_clear(FAIL_COL);
	}
		

	SleepThread();

	return 0;
}
