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
#include <dirent.h> // mkdir()
#include <unistd.h> // rmdir()

#include "../lib-usb/usb.h"
#include "../lib-bmp/bmp.h"
#include "../lib-read-fb/read-fb.h"

#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 256

framebuffer_t g_frame; // Frame buffer for even/odd lines
zbuffer_t g_z_enabled; // Z buffer
zbuffer_t g_z_disabled; // Z buffer

unsigned char z_data[4 * FRAME_HEIGHT * FRAME_WIDTH] __attribute__((aligned(64))); // For reading Z buffer back

#define COLOR_WAIT_USB 0x00FFFF
#define COLOR_WAIT_USB2 0x00A5FF
#define COLOR_FAIL 0x0000FF
#define COLOR_SUCCESS 0x00FF00
#define COLOR_DONE 0xFFFFFF

#define Z_BIAS 0xffffff00

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

	g_z_enabled.enable = DRAW_ENABLE;
	g_z_enabled.mask = 0;
	g_z_enabled.method = ZTEST_METHOD_ALLPASS;
	g_z_enabled.zsm = GS_ZBUF_32;
	g_z_enabled.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

	g_z_disabled = g_z_enabled;
	g_z_disabled.mask = 1; // Disable the Z buffer for this test

	graph_initialize_custom();
}

qword_t* my_draw_clear(qword_t* q, unsigned rgb)
{
	color_t bg_color;
	bg_color.r = rgb & 0xFF;
	bg_color.g = (rgb >> 8) & 0xFF;
	bg_color.b = (rgb >> 16) & 0xFF;
	bg_color.a = 0x80;
	bg_color.q = 1.0f;

	PACK_GIFTAG(q, GIF_SET_TAG(6, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GIF_SET_PRIM(PRIM_TRIANGLE_STRIP, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
	q++;
	PACK_GIFTAG(q, bg_color.rgbaq, GIF_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(0, 0, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(0xffff, 0, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(0, 0xffff, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(0xffff, 0xffff, 0), GIF_REG_XYZ2);
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

int render_test(int window_x, int window_y)
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
	
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 4096);
	qword_t* q;
	u64 *dw;

	// View the ZBuffer as a color buffer for debugging
	// graph_set_framebuffer(0, g_z_enabled.address, g_frame.width, g_frame.psm, 0, 0);

	q = packet;

	q = draw_setup_environment(q, 0, &g_frame, &g_z_enabled);

	q = draw_primitive_xyoffset(q, 0, window_x, window_y);
	
	q = draw_disable_tests(q, 0, &g_z_enabled);

	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_wait_fast();

	q = packet;
	
	q = my_draw_clear(q, 0);

	q = draw_framebuffer(q, 0, &g_frame);

	dw = (u64*)draw_prim_start(q, 0, &prim, &color);

	for (int i = 0; i < FRAME_HEIGHT; i++)
	{
		xyz_t xyz;
		xyz.x = 0;
		xyz.y = i << 4;
		xyz.z = Z_BIAS;
		*dw++ = xyz.xyz;

		*dw++ = color.rgbaq;

		xyz.x = 0xffff;
		xyz.y = i << 4;
		xyz.z = Z_BIAS + i;
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

	free(packet);
	return 0;
}

// int write_z_data_to_usb(const char* filename)
// {
// 	printf("Waiting for USB to be ready...\n");
// 	my_draw_clear_send(COLOR_WAIT_USB); // Clear screen to indicate start of USB operation

// 	printf("DEBUG %d\n", __LINE__);

// 	FlushCache(0); // We read data with DMA so flush before writing to USB

// 	reset_iop();
// 	load_irx_usb();

// 	if (wait_usb_ready() != 0)
// 	{
// 		my_draw_clear_send(COLOR_FAIL); // Red background if USB not ready
// 		printf("USB not ready!\n");
// 		return -1;
// 	}

// 	printf("USB is ready, writing %s...\n", filename);

// 	my_draw_clear_send(COLOR_WAIT_USB2);

// 	if (write_bmp(filename, z_data, FRAME_WIDTH, FRAME_HEIGHT, 32) != 0)
// 	{
// 		my_draw_clear_send(COLOR_FAIL); // Red background if BMP write failed
// 		printf("Failed to write %s!\n", filename);
// 		return -1;
// 	}

// 	my_draw_clear_send(COLOR_SUCCESS);

// 	printf("Z data written successfully to %s\n", filename);
// 	return 0;
// }

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	char filename[32];
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);
	int i;

	init_gs();

	// init_drawing_environment();

	for (i = 0; i < 4; i++)
	{

		memset(z_data, 0, sizeof(z_data));

		render_test(i * 1024, 0);

		_gs_glue_read_framebuffer(g_z_enabled.address, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, z_data);

		// Set the framebuffer back to the main frame buffer
		graph_set_framebuffer(0, g_frame.address, g_frame.width, g_frame.psm, 0, 0);

		// Subtract the bias from the Z buffer
		for (int j = 0; j < FRAME_HEIGHT * FRAME_WIDTH; j++)
		{
			u32 pixel = (u32)z_data[j * 4] | ((u32)z_data[j * 4 + 1] << 8) | ((u32)z_data[j * 4 + 2] << 16) | ((u32)z_data[j * 4 + 3] << 24);
			pixel -= Z_BIAS;
			z_data[j * 4] = pixel & 0xFF; // Red
			z_data[j * 4 + 1] = (pixel >> 8) & 0xFF; // Green
			z_data[j * 4 + 2] = (pixel >> 16) & 0xFF; // Blue
			z_data[j * 4 + 3] = 0xFF; // Alpha (just make opaque)
		}
		
		qword_t* q = packet;
		
		q = draw_setup_environment(q, 0, &g_frame, &g_z_disabled);

		q = draw_primitive_xyoffset(q, 0, 0, 0);

		q = draw_disable_tests(q, 0, &g_z_disabled);

		// Finish setting up the environment.
		q = draw_finish(q);


		dma_wait_fast();
		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		draw_wait_finish();

		sprintf(filename, "mass:z_data_%02d.bmp", i);

		if (write_bmp_to_usb(filename, z_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, my_draw_clear_send) != 0)
		{
			printf("Failed to write Z data to USB for iteration %d\n", i);
			break;
		}
	}

	if (i == 4)
	{
		my_draw_clear_send(COLOR_DONE);
		printf("Z precision test completed successfully.\n");
	}
	
	free(packet);
	SleepThread();

	return 0;
}
