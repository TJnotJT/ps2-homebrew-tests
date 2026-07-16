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

#include "../lib-common/common.h"
#include "../lib-read-fb/read-fb.h"
#include "../lib-bmp/bmp.h"
#include "../lib-usb/usb.h"

#define FRAME_WIDTH 256
#define FRAME_HEIGHT 256
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z; // Z buffer
blend_t g_blend;

u8 g_frame_data[FRAME_WIDTH * FRAME_HEIGHT * 4] __attribute__((aligned(64)));

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
	g_frame.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, GRAPH_ALIGN_PAGE);

	g_z.enable = DRAW_ENABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = 0;
	g_z.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

	g_blend.alpha = BLEND_ALPHA_SOURCE;
	g_blend.color1 = BLEND_COLOR_SOURCE;
	g_blend.color2 = BLEND_COLOR_DEST;
	g_blend.color3 = BLEND_COLOR_DEST;
	g_blend.fixed_alpha = 0;

	graph_initialize_custom();
}

qword_t *my_draw_disable_tests(qword_t *q, int context)
{
	PACK_GIFTAG(q, GIF_SET_TAG(1, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE, ATEST_METHOD_ALLPASS, 0x00, ATEST_KEEP_ALL,
							   DRAW_DISABLE, DRAW_DISABLE,
							   DRAW_ENABLE, ZTEST_METHOD_ALLPASS), GS_REG_TEST + context);
	q++;
	return q;

}

qword_t *my_draw_set_alpha_test(qword_t *q, int context)
{
	PACK_GIFTAG(q, GIF_SET_TAG(1, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE, ATEST_METHOD_NOTEQUAL, 0x80, ATEST_KEEP_ALL,
							   DRAW_DISABLE, DRAW_DISABLE,
							   DRAW_ENABLE, ZTEST_METHOD_ALLPASS), GS_REG_TEST + context);
	q++;
	return q;
}

qword_t* my_draw_clear(qword_t* q, unsigned rgb)
{
	color_t bg_color;
	bg_color.r = rgb & 0xFF;
	bg_color.g = (rgb >> 8) & 0xFF;
	bg_color.b = (rgb >> 16) & 0xFF;
	bg_color.a = 0xFF;
	bg_color.q = 1.0f;

	PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GIF_SET_PRIM(PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
	q++;
	PACK_GIFTAG(q, bg_color.rgbaq, GIF_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(WINDOW_X << 4, WINDOW_Y << 4, 0), GIF_REG_XYZ2);
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

qword_t* my_draw_triangle(qword_t* q)
{
	const int x = 16 * WINDOW_X;
	const int y = 16 * WINDOW_Y;

	const int x0 = x;
	const int y0 = y;
	const int x1 = x;
	const int y1 = y + FRAME_HEIGHT * 16;
	const int x2 = x + FRAME_WIDTH * 16;
	const int y2 = y;

  const int z = 0xFF00;

	color_t color;
	color.a = 0x80;
	color.r = 0xFF;
	color.g = 0;
	color.b = 0;
	color.q = 1.0f;

  PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_TRIANGLE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
  q++;

  PACK_GIFTAG(q, color.rgbaq, GS_REG_RGBAQ);
  q++;

  PACK_GIFTAG(q, GS_SET_XYZ(x0, y0, z), GS_REG_XYZ2);
  q++;

  PACK_GIFTAG(q, GS_SET_XYZ(x1, y1, z), GS_REG_XYZ2);
  q++;

	PACK_GIFTAG(q, GS_SET_XYZ(x2, y2, z), GS_REG_XYZ2);
	q++;

	return q;
}

int render_test(int frame_psm, int zbuf_zsm)
{
	srand(123);

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q;

	// Send a dummy EOP packet
	q = packet;
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	framebuffer_t frame = g_frame;
	zbuffer_t z = g_z;

	frame.psm = frame_psm;
	z.zsm = zbuf_zsm;

  // Set up the drawing environment and clear the frame buffer
	q = packet;
	q = draw_setup_environment(q, 0, &frame, &z);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_clear(q, 0);
	// q = my_draw_set_alpha_test(q, 0);
	q = my_draw_disable_tests(q, 0);
	q = draw_alpha_blending(q, 0, &g_blend);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	q = packet;
	q = my_draw_triangle(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	// Disable Z write so it doesn't get cleared.
	z.mask = 1;
	q = draw_setup_environment(q, 0, &frame, &z);

	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();
	graph_wait_vsync();

	free(packet);
	return 0;
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	
  init_gs();

	// C32 and Z32
	{
		render_test(GS_PSM_32, GS_PSMZ_32);

		// Readback color and write to file
		read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, g_frame_data);

		char filename[64];
		sprintf(filename, "mass:frame-zbuf-compat-1-c32.bmp");

		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, my_draw_clear_send) != 0)
			printf("Failed to write color data to USB\n");
		else
			printf("Wrote color data to USB\n");
		
		// Readback depth and write to file
		read_framebuffer(g_z.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, g_frame_data);
	
		sprintf(filename, "mass:frame-zbuf-compat-1-z32.bmp");
	
		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, my_draw_clear_send) != 0)
			printf("Failed to write depth data to USB\n");
		else
			printf("Wrote depth data to USB\n");
	}

	// C16 and Z32
	{
		render_test(GS_PSM_16, GS_PSMZ_32);

		// Readback color and write to file
		read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_16, g_frame_data);

		char filename[64];
		sprintf(filename, "mass:frame-zbuf-compat-2-c16.bmp");

		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_16, my_draw_clear_send) != 0)
			printf("Failed to write color data to USB\n");
		else
			printf("Wrote color data to USB\n");
		
		// Readback color and write to file
		read_framebuffer(g_z.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, g_frame_data);
	
		sprintf(filename, "mass:frame-zbuf-compat-2-z32.bmp");
	
		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, my_draw_clear_send) != 0)
			printf("Failed to write depth data to USB\n");
		else
			printf("Wrote depth data to USB\n");
	}

	// C32 and Z16
	{
		render_test(GS_PSM_32, GS_PSMZ_16);

		// Readback color and write to file
		read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, g_frame_data);

		char filename[64];
		sprintf(filename, "mass:frame-zbuf-compat-3-c32.bmp");

		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, my_draw_clear_send) != 0)
			printf("Failed to write color data to USB\n");
		else
			printf("Wrote color data to USB\n");
		
		read_framebuffer(g_z.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_16, g_frame_data);
	
		sprintf(filename, "mass:frame-zbuf-compat-3-z16.bmp");
	
		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_16, my_draw_clear_send) != 0)
			printf("Failed to write depth data to USB\n");
		else
			printf("Wrote depth data to USB\n");
	}

	// C16 and Z16
	{
		render_test(GS_PSM_16, GS_PSMZ_16);

		// Readback color and write to file
		read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_16, g_frame_data);

		char filename[64];
		sprintf(filename, "mass:frame-zbuf-compat-4-c16.bmp");

		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_16, my_draw_clear_send) != 0)
			printf("Failed to write color data to USB\n");
		else
			printf("Wrote color data to USB\n");
		
		read_framebuffer(g_z.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_16, g_frame_data);
	
		sprintf(filename, "mass:frame-zbuf-compat-4-z16.bmp");
	
		if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_16, my_draw_clear_send) != 0)
			printf("Failed to write depth data to USB\n");
		else
			printf("Wrote depth data to USB\n");
	}

	SleepThread();

	return 0;
}
