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
#include <string.h>
#include <dma.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <gs_privileged.h>
#include <gs_gp.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <math3d.h>

#include "usb.h"
#include "bmp.h"

#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 256
#define WINDOW_X (0)
#define WINDOW_Y (0)

framebuffer_t g_frame; // Frame buffer for even/odd lines
zbuffer_t g_z; // Z buffer

unsigned char z_data[3 * FRAME_HEIGHT * FRAME_WIDTH] __attribute__((aligned(64))); // For reading Z buffer back

#define COLOR_READING_Z 0x00FF00
#define COLOR_WAIT_USB 0x00FFFF
#define COLOR_WAIT_USB2 0x80FFFF
#define COLOR_FAIL 0x0000FF
#define COLOR_SUCCESS 0xFFFFFF

int graph_initialize_custom()
{
	int mode = graph_get_region();

	graph_set_mode(GRAPH_MODE_NONINTERLACED, mode, GRAPH_MODE_FRAME, GRAPH_ENABLE);

	graph_set_screen(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

	graph_set_bgcolor(0, 0, 0);

	graph_set_framebuffer(0, g_frame.address, g_frame.width, g_frame.psm, 0, 0);

	graph_set_output(1, 0, 1, 0, 1, 0xFF);

	return 0;
}

void init_gs()
{
	g_frame.width = FRAME_WIDTH;
	g_frame.height = FRAME_HEIGHT;
	g_frame.mask = 0;
	g_frame.psm = GS_PSM_32;
	g_frame.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, GRAPH_ALIGN_PAGE);

	// Enable the zbuffer.
	g_z.enable = DRAW_ENABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_GREATER_EQUAL;
	g_z.zsm = GS_ZBUF_24;
	g_z.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_24, GRAPH_ALIGN_PAGE);

	graph_initialize_custom();
}

void init_drawing_environment()
{
	qword_t *packet = aligned_alloc(64, sizeof(qword_t) * 128);
	qword_t *q = packet;

	q = draw_setup_environment(q, 0, &g_frame, &g_z);

	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	
	q = draw_disable_tests(q, 0, &g_z);

	// Finish setting up the environment.
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_wait_fast();

	free(packet);
}

qword_t* my_draw_clear(qword_t* q, unsigned rgb)
{
	color_t bg_color;
	bg_color.r = rgb & 0xFF;
	bg_color.g = (rgb >> 8) & 0xFF;
	bg_color.b = (rgb >> 16) & 0xFF;
	bg_color.a = 0x80;
	bg_color.q = 1.0f;

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
	return q;
}

void my_draw_clear_send(unsigned rgb)
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 32);
	qword_t* q = packet;
	q = my_draw_clear(q, rgb);
	q = draw_finish(q);

	dma_wait_fast();
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	draw_wait_finish();

	free(packet);
}

int render_test()
{
	prim_t prim;
	color_t color;

	color.r = 0xFF;
	color.g = 0xFF;
	color.b = 0xFF;
	color.a = 0x80;
	color.q = 1.0f;

	prim.type = PRIM_LINE;
	prim.shading = PRIM_SHADE_GOURAUD;
	prim.mapping = DRAW_DISABLE;
	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_DISABLE;
	prim.antialiasing = DRAW_DISABLE;
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;
	
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);
	qword_t* q;
	u64 *dw;

	for (;;)
	{

		// View the ZBuffer as a color buffer for debugging
		graph_set_framebuffer(0, g_z.address, g_frame.width, g_frame.psm, 0, 0);

		q = packet;
		
		q = my_draw_clear(q, 0);

		q = draw_framebuffer(q, 0, &g_frame);

		dw = (u64*)draw_prim_start(q, 0, &prim, &color);

		for (int i = 0; i < FRAME_HEIGHT / 2; i++)
		{
			xyz_t xyz;
			// xyz.x = WINDOW_X << 4;
			xyz.x = 0;
			xyz.y = (WINDOW_Y + i * 2) << 4;
			xyz.z = 0;
			*dw++ = xyz.xyz;

			*dw++ = color.rgbaq;

			xyz.x = (4096 << 4) - 1;
			// xyz.y = (WINDOW_Y + i * 2) << 4;
			xyz.z = i;
			*dw++ = xyz.xyz;
		}

		if ((u32)dw % 16)
			*dw++ = 0;

		q = draw_prim_end((qword_t *)dw, 3, GIF_REG_XYZ2 | (GIF_REG_RGBAQ << 4) | (GIF_REG_XYZ2 << 8));

		q = draw_finish(q);

		dma_wait_fast();
		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);

		draw_wait_finish();

		graph_wait_vsync();

		break;
	}

	// Set the framebuffer back to the main frame buffer
	// graph_set_framebuffer(0, g_frame.address, g_frame.width, g_frame.psm, 0, 0);

	free(packet);
	return 0;
}


// Taken from gs4ps2.c (https://github.com/F0bes/gs4ps2)
void gs_glue_read_fifo(u32 QWtotal, u128* data)
{
	volatile u32* VIF1CHCR = (u32*)0x10009000;
	volatile u32* VIF1MADR = (u32*)0x10009010;
	volatile u32* VIF1QWC = (u32*)0x10009020;
	volatile u128* VIF1FIFO = (u128*)0x10005000;
	volatile u32* VIF1_STAT = (u32*)0x10003C00;

	*VIF1_STAT = (1 << 23); // Set the VIF FIFO direction to VIF1 -> Main Memory
	*GS_REG_BUSDIR = 1;

	u32 QWrem = QWtotal;
	u128* data_curr = data;

	printf("Doing a readback of %d QW", QWtotal);
	while (QWrem >= 8) // Data transfers from the FIFO must be 128 byte aligned
	{
		*VIF1MADR = (u32)data_curr;
		u32 QWC = (((0xF000) < QWrem) ? (0xF000) : QWrem);
		QWC &= ~7;

		*VIF1QWC = QWC;
		QWrem -= QWC;
		data_curr += QWC;

		printf("    Reading chunk of %d QW. (Remaining %d)", QWC, QWrem);
		FlushCache(0);
		*VIF1CHCR = 0x100;
		asm __volatile__(" sync.l\n");
		while (*VIF1CHCR & 0x100)
		{
			//printf("VIF1CHCR %X\nVIF1STAT %X\nVIF1QWC %X\nGIF_STAT %X\n"
			//,*VIF1CHCR,*VIF1_STAT, *VIF1QWC, *(volatile u64*)0x10003020);
		};
	}
	printf("Finished off DMAC transfer, manually reading the %d QW from the fifo", QWrem);
	// Because we truncate the QWC, finish of the rest of the transfer
	// by reading the FIFO directly. Apparently this is how retail games do it.
	u32 qwFlushed = 0;
	while ((*VIF1_STAT >> 24))
	{
		printf("VIF1_STAT=%x", *VIF1_STAT);
		*data_curr = *VIF1FIFO;
		QWrem++;
		qwFlushed += 4;
		asm("nop\nnop\nnop\n");
	}
	printf("Finished off the manual read (%d QW)\n", qwFlushed);
	*GS_REG_BUSDIR = 0; // Reset BUSDIR
	*VIF1_STAT = 0; // Reset FIFO direction

	printf("Resetting TRXDIR");
	// Create a data packet that sets TRXDIR to 3, effectively cancelling whatever
	// transfer may be going on.
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 2);
	qword_t* q = packet;
	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	q->dw[0] = GS_SET_TRXDIR(3);
	q->dw[1] = GS_REG_TRXDIR;
	q++;
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	free(packet);
}

// Taken from gs4ps2.c (https://github.com/F0bes/gs4ps2)
void _gs_glue_read_framebuffer()
{
	*GS_REG_CSR = 2; // Clear any previous FINISH events

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 16);
	qword_t* q = packet;
	PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_FINISH(1), GS_REG_FINISH);
	q++;
	PACK_GIFTAG(q, GS_SET_BITBLTBUF(g_z.address >> 6, FRAME_WIDTH >> 6, GS_PSMZ_24, 0, 0, 0), GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXREG(FRAME_WIDTH, FRAME_HEIGHT), GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q, GS_SET_FINISH(1), GS_REG_FINISH);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXDIR(1), GS_REG_TRXDIR);
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	while (!(*GS_REG_CSR & 2))
		;


	gs_glue_read_fifo(sizeof(z_data) / sizeof(qword_t), (u128*)z_data);

	return;
}

// void read_z_data()
// {
// 	// FIXME: THIS WILL DESTROY Z BUFFER CONTENTS!
// 	// DISABLE Z BUFFER TESTS BEFORE READING!
// 	my_draw_clear_send(COLOR_READING_Z); // Clear screen to indicate start of Z data reading

// 	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 16);
// 	qword_t* q = packet;

// 	PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
// 	q++;
// 	PACK_GIFTAG(q, GS_SET_FINISH(1), GS_REG_FINISH);
// 	q++;
// 	PACK_GIFTAG(q, GS_SET_BITBLTBUF(g_z.address >> 6, FRAME_WIDTH >> 6, GS_PSMZ_24, 0, 0, 0), GS_REG_BITBLTBUF);
// 	q++;
// 	PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
// 	q++;
// 	PACK_GIFTAG(q, GS_SET_TRXREG(FRAME_WIDTH, FRAME_HEIGHT), GS_REG_TRXREG);
// 	q++;
// 	PACK_GIFTAG(q, GS_SET_TRXDIR(1), GS_REG_TRXDIR); // Start host -> local
// 	q++;

// 	// Send packet
// 	dma_wait_fast();
// 	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
// 	draw_wait_finish();

// 	// Now read from VIF1
// 	dma_channel_wait(DMA_CHANNEL_VIF1, 0);

// 	dma_channel_receive_normal(DMA_CHANNEL_VIF1, z_data, sizeof(z_data), 0, 0);

// 	dma_channel_wait(DMA_CHANNEL_VIF1, 0);

// 	free(packet);
// }

void write_z_data_to_usb()
{
	printf("Waiting for USB to be ready...\n");
	my_draw_clear_send(COLOR_WAIT_USB); // Clear screen to indicate start of USB operation

	printf("DEBUG %d\n", __LINE__);

	FlushCache(0); // We read data with DMA so flush before writing to USB

	reset_iop();
	load_irx_usb();

	if (wait_usb_ready() != 0)
	{
		my_draw_clear_send(COLOR_FAIL); // Red background if USB not ready
		printf("USB not ready!\n");
		return;
	}

	printf("USB is ready, writing z_data.bmp...\n");

	my_draw_clear_send(COLOR_WAIT_USB2); // Clear screen to indicate USB operation in progress

	// FILE* file = fopen("mass:z_data.bin", "wb");
	// if (file == NULL)
	// {
	// 	my_draw_clear_send(COLOR_FAIL); // Red background if file open failed
	// 	printf("Failed to open z_data.bin for writing!\n");
	// 	return;
	// }

	if (write_bmp("mass:z_data.bmp", z_data, FRAME_WIDTH, FRAME_HEIGHT, 24) != 0)
	{
		my_draw_clear_send(COLOR_FAIL); // Red background if BMP write failed
		printf("Failed to write z_data.bmp!\n");
		// fclose(file);
		return;
	}
	// size_t written = fwrite(z_data, 1, sizeof(z_data), file);
	// fflush(file); // Ensure data is written to disk
	// fclose(file);

	// if (written != sizeof(z_data))
	// {
	// 	my_draw_clear_send(COLOR_FAIL); // Red background if write failed
	// 	printf("Failed to write all data to z_data.bin!\n");
	// 	return;
	// }

	my_draw_clear_send(COLOR_SUCCESS);

	printf("Z data written successfully to mass:z_data.bmp\n");
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	memset(z_data, 0, sizeof(z_data));

	init_gs();

	init_drawing_environment();

	render_test();

	_gs_glue_read_framebuffer();

	int x, y;
	for (y = FRAME_HEIGHT - 1; y >= FRAME_HEIGHT - 6; y--)
	{
		for (x = FRAME_WIDTH - 1; x >= FRAME_WIDTH - 6; x--)
		{
			u32 rgb = (
				(z_data[(y * FRAME_WIDTH + x) * 3 + 0]) |
				(z_data[(y * FRAME_WIDTH + x) * 3 + 1] << 8) |
				(z_data[(y * FRAME_WIDTH + x) * 3 + 2] << 16)
			);
			printf("%08X ", rgb);
		}
		printf("\n");
	}

	// Set the framebuffer back to the main frame buffer
	graph_set_framebuffer(0, g_frame.address, g_frame.width, g_frame.psm, 0, 0);

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 32);
	qword_t* q = packet;
	g_z.enable = DRAW_DISABLE; // Disable Z buffer tests for the rest of the program
	
	q = draw_setup_environment(q, 0, &g_frame, &g_z);

	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	
	q = draw_disable_tests(q, 0, &g_z);

	// Finish setting up the environment.
	q = draw_finish(q);

	dma_wait_fast();
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	draw_wait_finish();

	free(packet);

	
	// SleepThread();
	// read_z_data();

	write_z_data_to_usb();

	SleepThread();

	return 0;
}
