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

#include "../lib-usb/usb.h"
#include "../lib-bmp/bmp.h"
#include "../lib-read-fb/read-fb.h"

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 448
#define TEX_SIZE 128
#define TEX_CHECKER_SIZE 8
#define TEX_COLOR_1 0x800000FF
#define TEX_COLOR_2 0x8000FF00
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)

framebuffer_t g_frame; // Frame buffer
framebuffer_t g_frame_dbg; // Debug frame buffer
zbuffer_t g_z; // Z buffer
texbuffer_t g_texbuf;

char TEXTURE_DATA[TEX_SIZE][TEX_SIZE][4] __attribute__((aligned(64)));

u8 frame_data[4 * FRAME_WIDTH * FRAME_HEIGHT] __attribute__((aligned(64)));

typedef union uv_t {
	struct {
		u16 u;
		u16 v;
		u32 pad;
	};
	u32 uv;
} uv_t __attribute__((aligned(16)));


void init_texture_data() {
  for (int x = 0; x < TEX_SIZE; x++) {
    for (int y = 0; y < TEX_SIZE; y++) {
      if (((x / TEX_CHECKER_SIZE) & 1) == 0) {
				if ((y & 1) == 0) {
					TEXTURE_DATA[y][x][0] = (TEX_COLOR_1 >> 0) & 0xFF;
					TEXTURE_DATA[y][x][1] = (TEX_COLOR_1 >> 8) & 0xFF;
					TEXTURE_DATA[y][x][2] = (TEX_COLOR_1 >> 16) & 0xFF;
					TEXTURE_DATA[y][x][3] = (TEX_COLOR_1 >> 24) & 0xFF;
				} else {
					TEXTURE_DATA[y][x][0] = (TEX_COLOR_2 >> 0) & 0xFF;
					TEXTURE_DATA[y][x][1] = (TEX_COLOR_2 >> 8) & 0xFF;
					TEXTURE_DATA[y][x][2] = (TEX_COLOR_2 >> 16) & 0xFF;
					TEXTURE_DATA[y][x][3] = (TEX_COLOR_2 >> 24) & 0xFF;
				}
      } else {
        TEXTURE_DATA[y][x][0] = 0x00;
        TEXTURE_DATA[y][x][1] = 0x00;
        TEXTURE_DATA[y][x][2] = 0x00;
        TEXTURE_DATA[y][x][3] = 0x80;
      }
    }
  }
}

void load_texture()
{
	qword_t *packet = aligned_alloc(64, sizeof(qword_t) * 128);

	qword_t *q;

	q = packet;

	FlushCache(0);

	q = draw_texture_transfer(q, TEXTURE_DATA, TEX_SIZE, TEX_SIZE, GS_PSM_32, g_texbuf.address, g_texbuf.width);
	q = draw_texture_flush(q);

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	free(packet);
}

void setup_texture()
{

	qword_t *packet = aligned_alloc(64, sizeof(qword_t) * 32);

	qword_t *q = packet;

	// Using a texture involves setting up a lot of information.
	clutbuffer_t clut;

	lod_t lod;

	lod.calculation = LOD_USE_K;
	lod.max_level = 0;
	lod.mag_filter = LOD_MAG_NEAREST;
	lod.min_filter = LOD_MIN_NEAREST;
	lod.l = 0;
	lod.k = 0;

	g_texbuf.info.width = draw_log2(TEX_SIZE);
	g_texbuf.info.height = draw_log2(TEX_SIZE);
	g_texbuf.info.components = TEXTURE_COMPONENTS_RGBA;
	g_texbuf.info.function = TEXTURE_FUNCTION_DECAL;

	clut.storage_mode = CLUT_STORAGE_MODE1;
	clut.start = 0;
	clut.psm = 0;
	clut.load_method = CLUT_NO_LOAD;
	clut.address = 0;

	q = draw_texture_sampling(q, 0, &lod);
	q = draw_texturebuffer(q, 0, &g_texbuf, &clut);

	// Now send the packet, no need to wait since it's the first.
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	free(packet);
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

	g_frame_dbg.width = FRAME_WIDTH;
	g_frame_dbg.height = FRAME_HEIGHT;
	g_frame_dbg.mask = 0;
	g_frame_dbg.psm = GS_PSM_32;
	g_frame_dbg.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_frame_dbg.psm, GRAPH_ALIGN_PAGE);

	g_z.enable = DRAW_DISABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = 0;
	g_z.address = 0;

  g_texbuf.width = TEX_SIZE;
  g_texbuf.psm = GS_PSM_32;
  g_texbuf.address = graph_vram_allocate(TEX_SIZE, TEX_SIZE, g_texbuf.psm, GRAPH_ALIGN_BLOCK);

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
	uv_t uv;

	color.r = 0xFF;
	color.g = 0x00;
	color.b = 0x00;
	color.a = 0x80;
	color.q = 1.0f;

	prim.type = PRIM_LINE;
	prim.shading = PRIM_SHADE_FLAT;
	prim.mapping = DRAW_ENABLE;
	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_DISABLE;
	prim.antialiasing = DRAW_DISABLE;
	prim.mapping_type = PRIM_MAP_UV;
	prim.colorfix = PRIM_UNFIXED;
	
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q;
	u64 *dw;

	// Send a dummy EOP packet
	q = packet;
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	init_texture_data();
	load_texture();
	setup_texture();

  // Set up the drawing environment
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame, &g_z);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_disable_tests(q, 0);
	// Put custom drawing environment settings here
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

		// draw prims
		dw = (u64*)draw_prim_start(q, 0, &prim, &color);
		for (int i = 0; i < FRAME_HEIGHT; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				uv.pad = (8 * j) << 4;
				uv.u = 0;
				uv.v = 8 + (j << 4);
				*dw++ = uv.uv;

				xyz.x = (WINDOW_X + FRAME_WIDTH / 2 + 8 * j) << 4;
				xyz.y = (WINDOW_Y + i) << 4; 
 				xyz.z = 0; 
 				*dw++ = xyz.xyz;

				uv.u = 8 << 4;
				uv.v = 8 + (j << 4);
				*dw++ = uv.uv;

				xyz.x = (WINDOW_X + FRAME_WIDTH / 2 + 8 * (j + 1)) << 4;
				xyz.y = (WINDOW_Y + i) << 4;
				xyz.z = 0;
				*dw++ = xyz.xyz;
			}
		}

		if ((u32)dw % 16)
			*dw++ = 0;

		q = draw_prim_end((qword_t *)dw, 2, GIF_REG_UV | (GIF_REG_XYZ2 << 4));

		q = draw_finish(q);

		dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		draw_wait_finish();
		graph_wait_vsync();
	} while (0); // Single iteration

	free(packet);
	return 0;
}

void render_debug(u32 color)
{
	graph_set_framebuffer(0, g_frame_dbg.address, g_frame_dbg.width, g_frame_dbg.psm, 0, 0);

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);
	qword_t* q;

	// Send a dummy EOP packet
	q = packet;
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	// Set up the debug drawing environment
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame_dbg, &g_z);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
  q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();
	graph_wait_vsync();

	// Draw the color
	q = packet;
	q = my_draw_clear(q, color);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();
	graph_wait_vsync();

	free(packet);
}


int main(int argc, char *argv[])
{
	printf("UV SIZE: %d\n", sizeof(uv_t));
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	
  init_gs();

	render_test();

	// _gs_glue_read_framebuffer(g_frame.address, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, frame_data);

	// write_bmp_to_usb("mass:frame.bmp", frame_data, FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, render_debug);

	SleepThread();

	return 0;
}
