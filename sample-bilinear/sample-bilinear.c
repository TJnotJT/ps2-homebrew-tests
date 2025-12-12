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

#define TYPE_COLOR 0
#define TYPE_DEPTH 1

#define FILTER_NEAREST 0
#define FILTER_LINEAR 1

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 256
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)

#define TEX_BUF_WIDTH 64
#define TEX_WIDTH 64
#define TEX_HEIGHT 32

framebuffer_t g_frame; // Frame buffer
framebuffer_t g_frame_src; // Frame buffer for source
zbuffer_t g_z; // Z buffer
zbuffer_t g_z_src; // Z buffer for source
texbuffer_t g_tex_color; // Texture buffer
texbuffer_t g_tex_depth; // Texture buffer
clutbuffer_t g_clut;
lod_t g_lod;
texwrap_t g_wrap;

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
	g_z.zsm = GS_ZBUF_32;
	g_z.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

	g_frame_src.width = TEX_BUF_WIDTH;
	g_frame_src.height = TEX_HEIGHT;
	g_frame_src.mask = 0;
	g_frame_src.psm = GS_PSM_32;
	g_frame_src.address = graph_vram_allocate(TEX_BUF_WIDTH, TEX_HEIGHT, GS_PSM_32, GRAPH_ALIGN_PAGE);

	g_z_src.enable = DRAW_ENABLE;
	g_z_src.mask = 0;
	g_z_src.method = ZTEST_METHOD_ALLPASS;	
	g_z_src.zsm = GS_ZBUF_32;
	g_z_src.address = graph_vram_allocate(TEX_BUF_WIDTH, TEX_HEIGHT, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

	g_tex_color.width = TEX_BUF_WIDTH;
	g_tex_color.psm = GS_PSM_32;
	g_tex_color.address = g_frame_src.address;

	g_tex_color.info.width = draw_log2(TEX_WIDTH);
	g_tex_color.info.height = draw_log2(TEX_HEIGHT);
	g_tex_color.info.function = TEXTURE_FUNCTION_DECAL;
	g_tex_color.info.components = TEXTURE_COMPONENTS_RGBA;

	g_tex_depth.width = TEX_BUF_WIDTH;
	g_tex_depth.psm = GS_PSMZ_32;
	g_tex_depth.address = g_z_src.address;

	g_tex_depth.info.width = draw_log2(TEX_WIDTH);
	g_tex_depth.info.height = draw_log2(TEX_HEIGHT);
	g_tex_depth.info.function = TEXTURE_FUNCTION_DECAL;
	g_tex_depth.info.components = TEXTURE_COMPONENTS_RGBA;

	g_clut.storage_mode = CLUT_STORAGE_MODE1;
	g_clut.start = 0;
	g_clut.psm = 0;
	g_clut.load_method = CLUT_NO_LOAD;
	g_clut.address = 0;

	g_lod.calculation = LOD_USE_K;
	g_lod.max_level = 0;
	g_lod.mag_filter = LOD_MAG_NEAREST;
	g_lod.min_filter = LOD_MAG_NEAREST;
	g_lod.l = 0;
	g_lod.k = 0;

	g_clut.storage_mode = CLUT_STORAGE_MODE1;
	g_clut.start = 0;
	g_clut.psm = 0;
	g_clut.load_method = CLUT_NO_LOAD;
	g_clut.address = 0;

	g_wrap.horizontal = WRAP_CLAMP;
	g_wrap.vertical = WRAP_CLAMP;
	g_wrap.minu = 0;
	g_wrap.maxu = 0 ;
	g_wrap.minv = 0;
	g_wrap.maxv = 0;

	graph_initialize_custom();
}

qword_t* setup_texture(qword_t* q, int type, int filter)
{
	lod_t lod = g_lod;
	lod.mag_filter = filter == FILTER_LINEAR ? LOD_MAG_LINEAR : LOD_MAG_NEAREST;
	lod.min_filter = filter == FILTER_LINEAR ? LOD_MIN_LINEAR : LOD_MIN_NEAREST;
	q = draw_texture_sampling(q, 0, &lod);
	q = draw_texturebuffer(q, 0, type == TYPE_DEPTH ? &g_tex_depth : &g_tex_color, &g_clut);
	q = draw_texture_wrapping(q, 0, &g_wrap);
	return q;
}

qword_t* my_draw_disable_tests(qword_t *q, int context)
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

int send_dummy_eop()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 32);
	qword_t* q = packet;

	// Send a dummy EOP packet
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	free(packet);
	return 0;
}

int render_src()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q;

	// Set up the drawing environment and clear the frame buffer
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame_src, &g_z_src);
	q = draw_primitive_xyoffset(q, 0, 0, 0);
	q = my_draw_clear(q, 0);
	q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	// Draw a checkerboard pattern
	for (int y = 0; y < TEX_HEIGHT; y += 1)
	{
		q = packet;
		
		PACK_GIFTAG(q, GIF_SET_TAG(TEX_WIDTH * 3, 0, 0, GS_PRIM_POINT, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		for (int x = 0; x < TEX_WIDTH; x += 1)
		{
			unsigned color = ((x + y) % 2) ? 0xFFFFFFFF : 0xFF000000;

			PACK_GIFTAG(q, GIF_SET_PRIM(PRIM_POINT, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
			q++;
			PACK_GIFTAG(q, color, GIF_REG_RGBAQ);
			q++;
			PACK_GIFTAG(q, GIF_SET_XYZ(x * 16, y * 16, color), GIF_REG_XYZ2);
			q++;
		}
		q = draw_finish(q);
		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		draw_wait_finish();
	}

	free(packet);
	return 0;
}

int render_test()
{
	srand(123);

	send_dummy_eop();

	render_src();

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q;

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

	// Draw a sprite magnifying the texture
	int y_offset = WINDOW_Y;
	for (int type = TYPE_COLOR; type <= TYPE_DEPTH; type++)
	{
		int x_offset = WINDOW_X;
		for (int filter = FILTER_NEAREST; filter <= FILTER_LINEAR; filter++)
		{
			q = packet;
			q = setup_texture(q, type, filter);
			PACK_GIFTAG(q, GIF_SET_TAG(7, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
			q++;
			PACK_GIFTAG(q, GIF_SET_PRIM(PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0), GIF_REG_PRIM);
			q++;
			PACK_GIFTAG(q, GIF_SET_UV(0, 0), GIF_REG_UV);
			q++;
			PACK_GIFTAG(q, 0x80808080, GIF_REG_RGBAQ);
			q++;
			PACK_GIFTAG(q, GIF_SET_XYZ(x_offset << 4, y_offset << 4, 0), GIF_REG_XYZ2);
			q++;
			PACK_GIFTAG(q, GIF_SET_UV((TEX_WIDTH - 2) << 4, (TEX_HEIGHT - 2) << 4), GIF_REG_UV);
			q++;
			PACK_GIFTAG(q, 0x80808080, GIF_REG_RGBAQ);
			q++;
			PACK_GIFTAG(q, GIF_SET_XYZ((x_offset + FRAME_WIDTH / 2 - 4) << 4, (y_offset + FRAME_HEIGHT / 2 - 4) << 4, 0), GIF_REG_XYZ2);
			q++;
			dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
			dma_channel_wait(DMA_CHANNEL_GIF, 0);

			x_offset += FRAME_WIDTH / 2;
		}
		y_offset += FRAME_HEIGHT / 2;
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
	sprintf(filename, "mass:sample_bilinear.bmp");

	if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, my_draw_clear_send) != 0)
		printf("Failed to write point test data to USB\n");
	else
		printf("Wrote point test data to USB\n");

	SleepThread();

	return 0;
}
