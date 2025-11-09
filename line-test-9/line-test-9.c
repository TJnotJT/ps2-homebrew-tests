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

#ifndef USE_TEX
#define USE_TEX 0
#endif

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)
#define TEST_REGION_SIZE 64
#define TEST_REGIONS_X 4
#define TEST_REGIONS_Y 4
#define TEST_REGIONS 16

#define TEX_BUF_WIDTH 256
#define TEX_WIDTH 256
#define TEX_HEIGHT 256

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z; // Z buffer
texbuffer_t g_tex; // Texture buffer
clutbuffer_t g_clut;
lod_t g_lod;
texwrap_t g_wrap;

u8 g_frame_data[FRAME_WIDTH * FRAME_HEIGHT * 4] __attribute__((aligned(64)));
u8 g_z_data[FRAME_WIDTH * FRAME_HEIGHT * 4] __attribute__((aligned(64)));
u8 g_texture_data[256 * 256 * 4] __attribute__((aligned(64)));

void make_texture()
{
	for (int y = 0; y < TEX_HEIGHT; y++)
	{
		for (int x = 0; x < TEX_WIDTH; x++)
		{
			int i = (y * TEX_WIDTH + x) * 4;
			g_texture_data[i + 0] = x;
			g_texture_data[i + 1] = y;
			g_texture_data[i + 2] = 0;
			g_texture_data[i + 3] = 0xFF;
		}
	}

	FlushCache(0); // Needed because we generate data in the EE.
}

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

	g_z.enable = DRAW_ENABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = GS_ZBUF_32;
	g_z.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

	g_tex.width = TEX_BUF_WIDTH;
	g_tex.psm = GS_PSM_32;
	g_tex.address = graph_vram_allocate(TEX_BUF_WIDTH, TEX_HEIGHT, GS_PSM_32, GRAPH_ALIGN_BLOCK);

	g_tex.info.width = draw_log2(TEX_WIDTH);
	g_tex.info.height = draw_log2(TEX_HEIGHT);
	g_tex.info.function = TEXTURE_FUNCTION_DECAL;
	g_tex.info.components = TEXTURE_COMPONENTS_RGBA;

	g_clut.storage_mode = CLUT_STORAGE_MODE1;
	g_clut.start = 0;
	g_clut.psm = 0;
	g_clut.load_method = CLUT_NO_LOAD;
	g_clut.address = 0;

	g_lod.calculation = LOD_USE_K;
	g_lod.max_level = 0;
	g_lod.mag_filter = LOD_MAG_NEAREST;
	g_lod.min_filter = LOD_MIN_NEAREST;
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

qword_t *setup_texture(qword_t* q)
{
	q = draw_texture_sampling(q, 0, &g_lod);
	q = draw_texturebuffer(q, 0, &g_tex, &g_clut);
	q = draw_texture_wrapping(q, 0, &g_wrap);

	return q;
}

void load_texture(texbuffer_t *texbuf)
{
	qword_t *packet = aligned_alloc(64, sizeof(qword_t) * 1024);

	qword_t *q = packet;

	q = draw_texture_transfer(q, g_texture_data, 1 << texbuf->info.width, 1 << texbuf->info.height,
		texbuf->psm, texbuf->address, texbuf->width);
	q = draw_texture_flush(q);

	FlushCache(0); // Needed because we generate data in the EE.

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, -1);

	free(packet);
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

qword_t* my_draw_line(qword_t* q, int region)
{
	unsigned use_aa = (region & 1) == 1;
	unsigned use_abe = (region & 2) == 2;
	unsigned alpha_index = (region >> 2) & 3;
	unsigned alpha_values[4] = { 64, 128, 192, 255 };
	unsigned color = (alpha_values[alpha_index] << 24) | 0x404040;

	const int x = WINDOW_X + TEST_REGION_SIZE * (region % TEST_REGIONS_X);
	const int y = WINDOW_Y + TEST_REGION_SIZE * (region / TEST_REGIONS_X);

	int fx0 = 16;
	int fy0 = 16 * (TEST_REGION_SIZE / 2) + 4;
	int fx1 = 16 * (TEST_REGION_SIZE - 2);
	int fy1 = 16 * (TEST_REGION_SIZE / 2) + 4;

	int U0 = rand() % (TEX_WIDTH * 16);
	int V0 = rand() % (TEX_HEIGHT * 16);
	int U1 = rand() % (TEX_WIDTH * 16);
	int V1 = rand() % (TEX_HEIGHT * 16);
	int X0 = (x + 1) * 16 + fx0;
	int Y0 = (y + 1) * 16 + fy0;
	int X1 = (x + 1) * 16 + fx1;
	int Y1 = (y + 1) * 16 + fy1;
	int Z0 = 0;
	int Z1 = 1;

  PACK_GIFTAG(q, GIF_SET_TAG(8, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_LINE, 1, 0, 0, use_abe, use_aa, 1, 0, 0), GIF_REG_PRIM);
  q++;

	PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 0, 0, 64), GS_REG_ALPHA);
	q++;

	PACK_GIFTAG(q, GS_SET_UV(U0, V0), GS_REG_UV);
	q++;
	
  PACK_GIFTAG(q, color, GS_REG_RGBAQ);
  q++;

  PACK_GIFTAG(q, GS_SET_XYZ(X0, Y0, Z0), GS_REG_XYZ2);
  q++;

	PACK_GIFTAG(q, GS_SET_UV(U1, V1), GS_REG_UV);
	q++;

	PACK_GIFTAG(q, color, GS_REG_RGBAQ);
  q++;

  PACK_GIFTAG(q, GS_SET_XYZ(X1, Y1, Z1), GS_REG_XYZ2);
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
	q = setup_texture(q);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_clear(q, 0);
	q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	q = packet;
	for (int region = 0; region < TEST_REGIONS; region++)
	{
		q = my_draw_line(q, region);
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

	make_texture();

	load_texture(&g_tex);

	render_test();

	read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, g_frame_data);

	read_framebuffer(g_z.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_32, g_z_data);

	char filename[64];
	sprintf(filename, "mass:line_test_9%s.bmp", USE_TEX ? "_tex" : "");

	if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, my_draw_clear_send) != 0)
		printf("Failed to write line test data to USB\n");
	else
		printf("Wrote line test data to USB\n");

	SleepThread();

	return 0;
}
