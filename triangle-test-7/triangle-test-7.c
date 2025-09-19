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

#ifndef USE_AA
#define USE_AA 0
#endif

#ifndef SWAP
#define SWAP 0
#endif

#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 1024
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)
#define TEST_REGION_WIDTH 9
#define TEST_REGION_HEIGHT 9
#define TEST_REGION_PAD 3
#define TEST_REGIONS_X ((FRAME_WIDTH - TEST_REGION_PAD) / (TEST_REGION_WIDTH + TEST_REGION_PAD))
#define TEST_REGIONS_Y ((FRAME_HEIGHT - TEST_REGION_PAD) / (TEST_REGION_HEIGHT + TEST_REGION_PAD))
#define TEST_REGIONS (TEST_REGIONS_X * TEST_REGIONS_Y)

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z; // Z buffer

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
	g_frame.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, GRAPH_ALIGN_PAGE);

	g_z.enable = DRAW_DISABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = 0;
	g_z.address = 0;

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

void get_rectangle_point(int region, int* x, int* y)
{
	if (0 <= region && region < TEST_REGION_WIDTH)
	{
		*x = region;
		*y = 0;
	}
	else if (TEST_REGION_WIDTH <= region && region < TEST_REGION_WIDTH + TEST_REGION_HEIGHT)
	{
		*x = TEST_REGION_WIDTH;
		*y = region - TEST_REGION_WIDTH;
	}
	else if (TEST_REGION_WIDTH + TEST_REGION_HEIGHT <= region && region < 2 * TEST_REGION_WIDTH + TEST_REGION_HEIGHT)
	{
		*x = (2 * TEST_REGION_WIDTH + TEST_REGION_HEIGHT) - region;
		*y = TEST_REGION_HEIGHT;
	}
	else if (2 * TEST_REGION_WIDTH + TEST_REGION_HEIGHT <= region && region < 2 * (TEST_REGION_WIDTH + TEST_REGION_HEIGHT))
	{
		*x = 0;
		*y = 2 * (TEST_REGION_WIDTH + TEST_REGION_HEIGHT) - region;
	}
	else
	{
		*x = 0;
		*y = 0;
	}
}

qword_t* my_draw_triangle(qword_t* q, int region)
{
	int region_x = region / TEST_REGIONS_Y;
	int region_y = region % TEST_REGIONS_Y;
	const int x = TEST_REGION_PAD + WINDOW_X + (TEST_REGION_WIDTH + TEST_REGION_PAD) * region_x;
	const int y = TEST_REGION_PAD + WINDOW_Y + (TEST_REGION_HEIGHT + TEST_REGION_PAD) * region_y;

	if (region_y > 2 * (TEST_REGION_WIDTH + TEST_REGION_HEIGHT)) 
		return q;
	
	if (region_x >= 10)
		return q;

	int offset = 8 * (region_x >= 5 ? 5 - region_x : region_x);
	
	int dx, dy;
	get_rectangle_point(region_y, &dx, &dy);

	int cx = TEST_REGION_WIDTH / 2;
	int cy = TEST_REGION_HEIGHT / 2;

	int x0, y0;
	int x1, y1;
	int x2, y2;

	x0 = cx; y0 = cy;
	x1 = cx; y1 = cy + 1;
	x2 = dx; y2 = dy;

	if (SWAP)
	{
		int tmp;
		tmp = x0; x0 = y0; y0 = tmp;
		tmp = x1; x1 = y1; y1 = tmp;
		tmp = x2; x2 = y2; y2 = tmp;
	}

	x0 += x;
	x1 += x;
	x2 += x;
	y0 += y;
	y1 += y;
	y2 += y;

  const int Z = 0;

	int X0, Y0;
	int X1, Y1;
	int X2, Y2;

	X0 = 16 * x0; Y0 = 16 * y0;
	X1 = 16 * x1; Y1 = 16 * y1;
	X2 = 16 * x2; Y2 = 16 * y2;

	if (SWAP)
	{
		Y1 += offset;
	}
	else
	{
		X1 += offset;
	}

  PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_TRIANGLE, 0, 0, 0, 0, USE_AA, 0, 0, 0), GIF_REG_PRIM);
  q++;

  PACK_GIFTAG(q, 0xFFFFFFFF, GS_REG_RGBAQ);
  q++;

  PACK_GIFTAG(q, GS_SET_XYZ(X0, Y0, Z), GS_REG_XYZ2);
  q++;

  PACK_GIFTAG(q, GS_SET_XYZ(X1, Y1, Z), GS_REG_XYZ2);
  q++;

	PACK_GIFTAG(q, GS_SET_XYZ(X2, Y2, Z), GS_REG_XYZ2);
	q++;

	return q;
}

int render_test()
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

  // Set up the drawing environment and clear the frame buffer
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame, &g_z);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_clear(q, 0);
	// q = my_draw_set_alpha_test(q, 0);
	q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	for (int region_x = 0; region_x < TEST_REGIONS_X; region_x++)
	{
		q = packet;
		for (int region_y = 0; region_y < TEST_REGIONS_Y; region_y++)
		{
			int region = region_x * TEST_REGIONS_Y + region_y;

			q = my_draw_triangle(q, region);
		}
		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
	}

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

	render_test();

	read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, g_frame_data);

	char filename[64];
	sprintf(filename, "mass:triangle_test_7%s%s.bmp", (USE_AA ? "_aa" : ""), (SWAP ? "_swap" : ""));

	if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, my_draw_clear_send) != 0)
		printf("Failed to write line test data to USB\n");
	else
		printf("Wrote line test data to USB\n");

	SleepThread();

	return 0;
}
