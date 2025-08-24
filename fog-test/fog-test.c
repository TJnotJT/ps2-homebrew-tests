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

#include "../lib-read-fb/read-fb.h"

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 448
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)

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

qword_t* draw_sprite(qword_t* q, int prim_rgb, int prim_f, int fog_rgb)
{
	PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 1, 0, 0, 0, 0, 0), GIF_REG_PRIM);
	q++;

	PACK_GIFTAG(q, GS_SET_FOG(fog_rgb & 0xFFFFFF), GS_REG_FOG);
	q++;

	u8 r = prim_rgb & 0xFF;
	u8 g = (prim_rgb >> 8) & 0xFF;
	u8 b = (prim_rgb >> 16) & 0xFF;
	u8 a = 0x80;
	float Q = 1.0f;
	u32 Q_bits = *(u32*)&Q;

	PACK_GIFTAG(q, GIF_SET_RGBAQ(r, g, b, a, Q_bits), GIF_REG_RGBAQ);
	q++;

	u32 x0 = WINDOW_X << 4, x1 = (WINDOW_X + FRAME_WIDTH) << 4;
	u32 y0 = WINDOW_Y << 4, y1 = (WINDOW_Y + FRAME_HEIGHT) << 4;
	u32 z0 = 0, z1 = 0;

	PACK_GIFTAG(q, GIF_SET_XYZF(x0, y0, z0, prim_f & 0xFF), GIF_REG_XYZF2);
	q++;

	PACK_GIFTAG(q, GIF_SET_XYZF(x1, y1, z1, prim_f & 0xFF), GIF_REG_XYZF2);
	q++;

	return q;
}

int expected_final_rgb(int prim_rgb, int prim_f, int fog_rgb)
{
	const int prim_chan[3] = { (prim_rgb >> 0) & 0xFF, (prim_rgb >> 8) & 0xFF, (prim_rgb >> 16) & 0xFF };
	const int fog_chan[3] = {  (fog_rgb >> 0) & 0xFF, (fog_rgb >> 8) & 0xFF, (fog_rgb >> 16) & 0xFF };
	prim_f &= 0xFF;

	int expected_chan[3];
	for (int i = 0; i < 3; i++)
	{
		int chan = ((prim_chan[i] * prim_f) >> 8) + ((fog_chan[i] * (0xFF - prim_f)) >> 8);
		expected_chan[i] = chan;
	}
	return (expected_chan[0] << 0) | (expected_chan[1] << 8) | (expected_chan[2] << 16);
}

int render_test(u32 seed, u32 n_prims)
{
	srand(seed);
	
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q;

	// Send a dummy EOP packet
	q = packet;
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

  // Set up the drawing environment
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame, &g_z);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	for (int i = 0; i < n_prims; i++)
	{
		q = packet;

		const int rgb = rand() & 0xFFFFFF;
		const int fog_rgb = rand() & 0xFFFFFF;
		const int f = rand() & 0xFF;
		const int expected = expected_final_rgb(rgb, f, fog_rgb);
		q = draw_sprite(q, rgb, f, fog_rgb);
		q = draw_finish(q);

		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		draw_wait_finish();

		_gs_glue_read_framebuffer(g_frame.address, g_frame.width, g_frame.height, g_frame.psm, g_frame_data);

		// Take just the first pixel; they should all be the same.
		const int actual = (g_frame_data[0] << 0) | (g_frame_data[1] << 8) | (g_frame_data[2] << 16);

		const char *msg = (expected == actual) ? "PASS" : "FAIL";

		printf("\n%s RGB=%06X F=%02X FOG=%06X => Expected=%06X Actual=%06X\n", msg,
			rgb, f, fog_rgb, expected, actual);

		if (expected != actual)
			break;
	}

	free(packet);
	return 0;
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	
  init_gs();

	render_test(123, __UINT32_MAX__);

	SleepThread();

	return 0;
}
