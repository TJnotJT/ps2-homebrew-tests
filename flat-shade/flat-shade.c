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

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 448
#define TEX_SIZE 128
#define TEX_CHECKER_SIZE 8
#define TEX_COLOR 0x800000FF
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z; // Z buffer

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

int render_test()
{
	prim_t prim;
	color_t color;
	xyz_t xyz;

	color.r = 0xFF;
	color.g = 0x00;
	color.b = 0x00;
	color.a = 0x80;
	color.q = 1.0f;
	
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q;
	u64 *dw;

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
	graph_wait_vsync();

	do
	{
		// Clear
		q = packet;
		q = my_draw_clear(q, 0x80ffffff);

		// Draw triangle
		prim.type = PRIM_TRIANGLE;
		prim.shading = PRIM_SHADE_FLAT;
		prim.mapping = DRAW_ENABLE;
		prim.fogging = DRAW_DISABLE;
		prim.blending = DRAW_DISABLE;
		prim.antialiasing = DRAW_DISABLE;
		prim.mapping_type = PRIM_MAP_UV;
		prim.colorfix = PRIM_UNFIXED;
		dw = (u64*)draw_prim_start(q, 0, &prim, &color);
		
		// Vertex 1
		color.r = 0xFF;
		color.g = 0x00;
		color.b = 0x00;
		color.a = 0x80;
		color.q = 1.0f;
		*dw++ = color.rgbaq;

		xyz.x = (WINDOW_X + 64) << 4;
		xyz.y = (WINDOW_Y + 64) << 4;
		xyz.z = 0;
		*dw++ = xyz.xyz;

		// Vertex 2
		color.r = 0x00;
		color.g = 0xFF;
		color.b = 0x00;
		color.a = 0x80;
		color.q = 1.0f;
		*dw++ = color.rgbaq;

		xyz.x = (WINDOW_X + 64 + 32) << 4;
		xyz.y = (WINDOW_Y + 64) << 4;
		xyz.z = 0;
		*dw++ = xyz.xyz;

		// Vertex 3
		color.r = 0x00;
		color.g = 0x00;
		color.b = 0xFF;
		color.a = 0x80;
		color.q = 1.0f;
		*dw++ = color.rgbaq;

		xyz.x = (WINDOW_X + 64 + 32) << 4;
		xyz.y = (WINDOW_Y + 64 + 32) << 4;
		xyz.z = 0;
		*dw++ = xyz.xyz;

		if ((u32)dw % 16)
			*dw++ = 0;

		q = draw_prim_end((qword_t *)dw, 2, GIF_REG_RGBAQ | (GIF_REG_XYZ2 << 4));

		// Draw line
		prim.type = PRIM_LINE;
		prim.shading = PRIM_SHADE_FLAT;
		prim.mapping = DRAW_ENABLE;
		prim.fogging = DRAW_DISABLE;
		prim.blending = DRAW_DISABLE;
		prim.antialiasing = DRAW_DISABLE;
		prim.mapping_type = PRIM_MAP_UV;
		prim.colorfix = PRIM_UNFIXED;
		dw = (u64*)draw_prim_start(q, 0, &prim, &color);
		
		// Vertex 1
		color.r = 0xFF;
		color.g = 0x00;
		color.b = 0x00;
		color.a = 0x80;
		color.q = 1.0f;
		*dw++ = color.rgbaq;

		xyz.x = (WINDOW_X + 64) << 4;
		xyz.y = (WINDOW_Y + 128) << 4;
		xyz.z = 0;
		*dw++ = xyz.xyz;

		// Vertex 2
		color.r = 0x00;
		color.g = 0xFF;
		color.b = 0x00;
		color.a = 0x80;
		color.q = 1.0f;
		*dw++ = color.rgbaq;

		xyz.x = (WINDOW_X + 64 + 32) << 4;
		xyz.y = (WINDOW_Y + 128 + 32) << 4;
		xyz.z = 0;
		*dw++ = xyz.xyz;

		if ((u32)dw % 16)
			*dw++ = 0;

		q = draw_prim_end((qword_t *)dw, 2, GIF_REG_RGBAQ | (GIF_REG_XYZ2 << 4));

		q = draw_finish(q);

		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		draw_wait_finish();
		graph_wait_vsync();

	} while (1);

	free(packet);
	return 0;
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	
  init_gs();

	render_test();

	SleepThread();

	return 0;
}
