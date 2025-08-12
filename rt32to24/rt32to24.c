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
#define NPOS_X 4
#define NPOS_Y 4
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)
#define DRAW_COLOR 0xFFFFFFFF
#define CLEAR_COLOR 0x00000000
#define DRAW_Z 0xFFFFFFFF

framebuffer_t g_frame32; // Frame buffer
framebuffer_t g_frame24; // Frame buffer
zbuffer_t g_z32; // Z buffer
zbuffer_t g_z24; // Z buffer
framebuffer_t g_z_as_frame32; // Z buffer as framebuffer
framebuffer_t g_z_as_frame24; // Z buffer as framebuffer

int graph_initialize_custom()
{
	int mode = graph_get_region();

	graph_set_mode(GRAPH_MODE_NONINTERLACED, mode, GRAPH_MODE_FRAME, GRAPH_ENABLE);

	graph_set_screen(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

	graph_set_bgcolor(0, 0, 0);

	graph_set_framebuffer(0, g_frame32.address, g_frame32.width, g_frame32.psm, 0, 0);

	graph_set_output(1, 0, 1, 0, 1, 0xFF);

	return 0;
}

void init_gs()
{
	g_frame32.width = FRAME_WIDTH;
	g_frame32.height = FRAME_HEIGHT;
	g_frame32.mask = 0;
	g_frame32.psm = GS_PSM_32;
	g_frame32.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_frame32.psm, GRAPH_ALIGN_PAGE);

	g_frame24.width = FRAME_WIDTH;
	g_frame24.height = FRAME_HEIGHT;
	g_frame24.mask = 0;
	g_frame24.psm = GS_PSM_24;
	g_frame24.address = g_frame32.address;

	g_z32.enable = DRAW_ENABLE;
	g_z32.mask = 0;
	g_z32.method = ZTEST_METHOD_ALLPASS;
	g_z32.zsm = GS_ZBUF_32;
	g_z32.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_z32.zsm, GRAPH_ALIGN_PAGE);

	g_z24.enable = DRAW_ENABLE;
	g_z24.mask = 0;
	g_z24.method = ZTEST_METHOD_ALLPASS;
	g_z24.zsm = GS_ZBUF_24;
	g_z24.address = g_z32.address;

	g_z_as_frame32.width = FRAME_WIDTH;
	g_z_as_frame32.height = FRAME_HEIGHT;
	g_z_as_frame32.mask = 0;
	g_z_as_frame32.psm = GS_PSM_32;
	g_z_as_frame32.address = g_z32.address;

	g_z_as_frame24.width = FRAME_WIDTH;
	g_z_as_frame24.height = FRAME_HEIGHT;
	g_z_as_frame24.mask = 0;
	g_z_as_frame24.psm = GS_PSM_24;
	g_z_as_frame24.address = g_z24.address;

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

qword_t* my_draw_clear(qword_t* q, unsigned rgba)
{
	color_t bg_color;
	bg_color.r = (rgba >> 0) & 0xFF;
	bg_color.g = (rgba >> 8) & 0xFF;
	bg_color.b = (rgba >> 16) & 0xFF;
	bg_color.a = (rgba >> 24) & 0xFF;
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

qword_t* draw_sprite(qword_t* q, int pos)
{
	PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_PRIM(PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
	q++;

	u32 base_x = (WINDOW_X << 4) + (pos % NPOS_X) * (FRAME_WIDTH << 4) / NPOS_X;
	u32 base_y = (WINDOW_Y << 4) + (pos / NPOS_X) * (FRAME_HEIGHT << 4) / NPOS_Y;
	u32 z = DRAW_Z;

	for (int i = 0; i < 2; i++)
	{
		u8 r = (DRAW_COLOR >> 0) & 0xFF;
		u8 g = (DRAW_COLOR >> 8) & 0xFF;
		u8 b = (DRAW_COLOR >> 16) & 0xFF;
		u8 a = (DRAW_COLOR >> 24) & 0xFF;
		float f = 1.0f;
		u32 f_bits = *(u32*)&f;

		PACK_GIFTAG(q, GIF_SET_RGBAQ(r, g, b, a, f_bits), GIF_REG_RGBAQ);
		q++;

		u32 x = base_x + i * (FRAME_WIDTH << 4) / NPOS_X;
		u32 y = base_y + i * (FRAME_HEIGHT << 4) / NPOS_Y;

		PACK_GIFTAG(q, GIF_SET_XYZ(x, y, z), GIF_REG_XYZ2);
		q++;
	}
	return q;
}

qword_t* render_sprite(qword_t* packet, int pos, u32 n_prims, u32 use32, u32 clear, u32 z_as_frame)
{
	qword_t* q = packet;

	framebuffer_t frame = z_as_frame ? (use32 ? g_z_as_frame32 : g_z_as_frame24) : (use32 ? g_frame32 : g_frame24);
	zbuffer_t z = use32 ? g_z32 : g_z24;

	z.enable = !z_as_frame;

	// Set up the drawing environment
	q = draw_setup_environment(q, 0, &frame, &z);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);

	if (clear)
	{
		q = my_draw_clear(q, CLEAR_COLOR);
		q = draw_finish(q);
	}

	for (int i = 0; i < n_prims; i++)
	{
		q = draw_sprite(q, pos); 
	}
	q = draw_finish(q);

	return q;
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	
  init_gs();

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q = packet;

	// Send a dummy EOP packet
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();
	while (1)
	{
		srand(123);

		q = packet;

		// Drawing to the regular framebuffer
		q = render_sprite(q, 0, 1, 1, 1, 0); // 32 bit frame and z
		q = render_sprite(q, 1, 1, 0, 0, 0); // 24 bit frame and z
		q = render_sprite(q, 2, 1, 1, 0, 0); // 32 bit frame and z

		// Drawing to the Z buffer as framebuffer
		q = render_sprite(q, 3, 1, 1, 0, 1); // 32 bit z as frame
		q = render_sprite(q, 4, 1, 0, 0, 1); // 24 bit z as frame
		q = render_sprite(q, 5, 1, 1, 0, 1); // 32 bit z as frame

		// Drawing to the regular framebuffer again
		q = render_sprite(q, 6, 1, 1, 0, 0); // 32 bit frame and z
		q = render_sprite(q, 7, 1, 0, 0, 0); // 24 bit frame and z
		q = render_sprite(q, 8, 1, 1, 0, 0); // 32 bit frame and z

		// Drawing to the Z buffer as framebuffer
		q = render_sprite(q, 9, 1, 0, 0, 1); // 24 bit z as frame

		// Drawing to the regular framebuffer again
		q = render_sprite(q, 10, 1, 0, 0, 0); // 24 bit frame and z
		q = render_sprite(q, 11, 1, 1, 0, 0); // 32 bit frame and z
		q = render_sprite(q, 12, 1, 0, 0, 0); // 24 bit frame and z

		// Drawing to the Z buffer as framebuffer
		q = render_sprite(q, 13, 1, 0, 0, 1); // 32 bit z as frame

		// Drawing to the regular framebuffer again
		q = render_sprite(q, 14, 1, 0, 0, 0); // 32 bit frame and z


		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		draw_wait_finish();
		graph_wait_vsync();
	}
	free(packet);
	
	SleepThread();

	return 0;
}
