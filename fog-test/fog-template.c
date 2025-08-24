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

#ifndef TEST_MODE
#error "TEST_MODE must be defined to 0 or 1"
#endif


#define FRAME_WIDTH 512
#define FRAME_HEIGHT 448
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)
#define TEST_REGION_WIDTH 5
#define TEST_REGIONS (FRAME_WIDTH * FRAME_WIDTH / TEST_REGION_WIDTH)
#define TYPE_VERT_RGB 0
#define TYPE_VERT_F 1
#define TYPE_FOG_RGB 2
#define TYPE_VERT_FOG_RGB 3

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


int expected_final_rgb(int prim_rgb, int prim_f, int fog_rgb)
{
	const int prim_chan[3] = { (prim_rgb >> 0) & 0xFF, (prim_rgb >> 8) & 0xFF, (prim_rgb >> 16) & 0xFF };
	const int fog_chan[3] = { (fog_rgb >> 0) & 0xFF, (fog_rgb >> 8) & 0xFF, (fog_rgb >> 16) & 0xFF };
	prim_f &= 0xFF;

	int expected_chan[3];
	for (int i = 0; i < 3; i++)
		expected_chan[i] = (prim_chan[i] * prim_f + fog_chan[i] * (0x100 - prim_f)) >> 8;
	return (expected_chan[0] << 0) | (expected_chan[1] << 8) | (expected_chan[2] << 16);
}

qword_t* my_draw_point(qword_t* q, int region, int type, int vert_rgb, int vert_f, int fog_rgb)
{
	// 4 vertical pixels per column and 1 blank
	const int x = WINDOW_X + TEST_REGION_WIDTH * (region / FRAME_HEIGHT) + type;
	const int y = WINDOW_Y + region % FRAME_HEIGHT;
	const int z = 0;

	if (type == TYPE_VERT_RGB)
	{
		PACK_GIFTAG(q, GIF_SET_TAG(3, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;

		PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_POINT, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
		q++;

		PACK_GIFTAG(q, (vert_rgb & 0xFFFFFF) | 0xFF000000, GS_REG_RGBAQ);
		q++;

		PACK_GIFTAG(q, GS_SET_XYZ(x << 4, y << 4, z), GS_REG_XYZ2);
		q++;
	}
	else if (type == TYPE_VERT_F)
	{
		PACK_GIFTAG(q, GIF_SET_TAG(3, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;

		PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_POINT, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
		q++;

		PACK_GIFTAG(q, (vert_f & 0xFF) | 0xFF000000, GS_REG_RGBAQ);
		q++;

		PACK_GIFTAG(q, GS_SET_XYZ(x << 4, y << 4, z), GS_REG_XYZ2);
		q++;
	}
	else if (type == TYPE_FOG_RGB)
	{
		PACK_GIFTAG(q, GIF_SET_TAG(3, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;

		PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_POINT, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
		q++;

		PACK_GIFTAG(q, (fog_rgb & 0xFFFFFF) | 0xFF000000, GS_REG_RGBAQ);
		q++;

		PACK_GIFTAG(q, GS_SET_XYZ(x << 4, y << 4, z), GS_REG_XYZ2);
		q++;
	}
	else if (type == TYPE_VERT_FOG_RGB && TEST_MODE == 0)
	{
		PACK_GIFTAG(q, GIF_SET_TAG(3, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;

		PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_POINT, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
		q++;

		PACK_GIFTAG(q, (expected_final_rgb(vert_rgb, vert_f, fog_rgb) & 0xFFFFFF) | 0xFF000000, GS_REG_RGBAQ);
		q++;

		PACK_GIFTAG(q, GS_SET_XYZ(x << 4, y << 4, z), GS_REG_XYZ2);
		q++;
	}
	else if (type == TYPE_VERT_FOG_RGB && TEST_MODE == 1)
	{
		PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;

		PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_POINT, 0, 0, 1, 0, 0, 0, 0, 0), GS_REG_PRIM);
		q++;

		PACK_GIFTAG(q, fog_rgb & 0xFFFFFF, GS_REG_FOGCOL);
		q++;

		PACK_GIFTAG(q, (vert_rgb & 0xFFFFFF) | 0xFF000000, GS_REG_RGBAQ);
		q++;

		PACK_GIFTAG(q, GS_SET_XYZF(x << 4, y << 4, z, vert_f & 0xFF), GS_REG_XYZF2);
		q++;
	}
	else
	{
		printf("Unknown type: %d\n", type);
	}

	return q;
}

int render_test(u32 seed)
{
	my_srand(seed);
	
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
	q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();
	
	for (int region = 0; region < TEST_REGIONS; region++)
	{
		const int vert_rgb = my_rand() & 0xFFFFFF;
		const int vert_f = my_rand() & 0xFF;
		const int fog_rgb = my_rand() & 0xFFFFFF;

		for (int type = TYPE_VERT_RGB; type <= TYPE_VERT_FOG_RGB; type++)
			q = my_draw_point(q, region, type, vert_rgb, vert_f, fog_rgb);

		if ((region > 0) && (region % FRAME_HEIGHT == 0))
		{
			dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
			dma_channel_wait(DMA_CHANNEL_GIF, 0);

			q = packet;
		}
	}
	
	if (q != packet)
	{
		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);

		q = packet;
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

	render_test(123);

	_gs_glue_read_framebuffer(g_frame.address, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, g_frame_data);

	char filename[64];
	sprintf(filename, "mass:fog_data.bmp");

	if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, my_draw_clear_send) != 0)
		printf("Failed to write fog data to USB\n");
	else
		printf("Wrote fog data to USB\n");

	SleepThread();

	return 0;
}
