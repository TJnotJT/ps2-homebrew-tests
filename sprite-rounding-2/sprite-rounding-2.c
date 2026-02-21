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
#include "../lib-common/common.h"

#ifndef PREFIX
#define PREFIX "sprite-rounding-2"
#endif

#ifndef CLMP
#define CLMP 0
#endif

#ifndef SHF
#define SHF 0
#endif

#ifndef ROT
#define ROT 0
#endif

#ifndef REV
#define REV 0
#endif

#ifndef OFFSET_U
#define OFFSET_U 0
#endif

#ifndef OFFSET_FRAC
#define OFFSET_FRAC 0
#endif

#ifndef TW
#define TW 4
#endif

#ifndef TBW
#define TBW 1
#endif

#if ROT
#define FRAME_WIDTH 256
#define FRAME_HEIGHT 1024
#else
#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 256
#endif

#ifndef TRI
#define TRI 0
#endif

#define WINDOW_X 0
#define WINDOW_Y 0
#define TEX_BUFF_WIDTH (64 * (TBW))
#define TEX_SIZE (1 << (TW))
#define TEX_SIZE (1 << (TW))
#define REGION_SIZE_X 32
#define REGION_SIZE_Y 16
#define NREGION_X (FRAME_WIDTH / REGION_SIZE_X)
#define NREGION_Y (FRAME_HEIGHT / REGION_SIZE_Y)

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z; // Z buffer
texbuffer_t g_tex; // Texture buffer
clutbuffer_t g_clut;
lod_t g_lod;
texwrap_t g_wrap;

u8 g_frame_data[FRAME_WIDTH * FRAME_HEIGHT * 4] __attribute__((aligned(64)));

u8 texture_data[TEX_SIZE * TEX_SIZE * 4] __attribute__((aligned(64)));

// Checkerboard texture
void make_texture()
{
	for (int i = 0; i < TEX_SIZE; i++)
	{
		for (int j = 0; j < TEX_SIZE; j++)
		{
			int addr = ROT ? (j * TEX_SIZE + i) : (i * TEX_SIZE + j);
			texture_data[4 * addr + 0] = i & 0xFF; // R
			texture_data[4 * addr + 1] = j & 0xFF; // G
			texture_data[4 * addr + 2] = 0; // B
			texture_data[4 * addr + 3] = 0xFF; // A
		}
	}
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

	g_z.enable = DRAW_DISABLE;
	g_z.mask = 1;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = GS_PSMZ_32;
	g_z.address = 0;

	g_tex.width = TEX_BUFF_WIDTH;
	g_tex.psm = GS_PSM_32;
	g_tex.address = graph_vram_allocate(g_tex.width, TEX_SIZE, GS_PSM_32, GRAPH_ALIGN_BLOCK);

	g_tex.info.width = draw_log2(TEX_SIZE);
	g_tex.info.height = draw_log2(TEX_SIZE);
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

	g_wrap.horizontal = CLMP ? WRAP_CLAMP : WRAP_REPEAT;
	g_wrap.vertical = CLMP ? WRAP_CLAMP : WRAP_REPEAT;
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

	q = draw_texture_transfer(q, texture_data, 1 << texbuf->info.width, 1 << texbuf->info.height,
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

qword_t* my_draw_sprite(qword_t* q, int region_x, int region_y)
{
	PACK_GIFTAG(q, GIF_SET_TAG(TRI ? 10 : 6, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_PRIM(TRI ? GS_PRIM_TRIANGLE_STRIP : GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0), GS_REG_PRIM);
	q++;

	u32 cx = region_x * REGION_SIZE_X;
	u32 cy = region_y * REGION_SIZE_Y;

	u32 x0 = ((WINDOW_X + cx + SHF) << 4) + OFFSET_FRAC;
	u32 x1 = ((WINDOW_X + SHF + region_x + cx + 1) << 4) + OFFSET_FRAC;
	u32 y0 = (WINDOW_Y + cy) << 4;
	u32 y1 = (WINDOW_Y + region_y + cy + 1) << 4;
	u32 z0 = 0;
	u32 z1 = 0;
	
	u32 u0 = (OFFSET_U << 4) + OFFSET_FRAC;
	u32 u1 = ((region_x + 1 + OFFSET_U) << 4) + OFFSET_FRAC;
	u32 v0 = 0;
	u32 v1 = (region_y + 1) << 4;

	if (REV)
	{
		u32 tmp = u0;
		u0 = u1;
		u1 = tmp;
	}

	if (ROT)
	{
		u32 tmp = x0;
		x0 = y0;
		y0 = tmp;

		tmp = x1;
		x1 = y1;
		y1 = tmp;

		tmp = u0;
		u0 = v0;
		v0 = tmp;

		tmp = u1;
		u1 = v1;
		v1 = tmp;
	}

	color_t rgba;
	rgba.a = 0xFF;
	rgba.g = 0xFF;
	rgba.b = 0xFF;
	rgba.a = 0xFF;
	rgba.q = 1.0f;

	PACK_GIFTAG(q, rgba.rgbaq, GIF_REG_RGBAQ);
	q++;

	PACK_GIFTAG(q, GIF_SET_UV(u0, v0), GIF_REG_UV);
	q++;

	PACK_GIFTAG(q, GIF_SET_XYZ(x0, y0, z0), GIF_REG_XYZ2);
	q++;

	if (TRI)
	{
		PACK_GIFTAG(q, GIF_SET_UV(u1, v0), GIF_REG_UV);
		q++;

		PACK_GIFTAG(q, GIF_SET_XYZ(x1, y0, z0), GIF_REG_XYZ2);
		q++;
	}

	if (TRI)
	{
		PACK_GIFTAG(q, GIF_SET_UV(u0, v1), GIF_REG_UV);
		q++;

		PACK_GIFTAG(q, GIF_SET_XYZ(x0, y1, z0), GIF_REG_XYZ2);
		q++;
	}

	PACK_GIFTAG(q, GIF_SET_UV(u1, v1), GIF_REG_UV);
	q++;

	PACK_GIFTAG(q, GIF_SET_XYZ(x1, y1, z1), GIF_REG_XYZ2);
	q++;

	return q;
}

int render_test()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 16384);
	qword_t* q;

	// *GS_REG_CSR = 2; // Clear previous finish event.

	// Send a dummy EOP packet
	q = packet;
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

  // Set up the drawing environment
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame, &g_z);
	q = setup_texture(q);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_disable_tests(q, 0);
	q = my_draw_clear(q, 0xff0000);

	// Draw a textured sprite
	for (int i = 0; i < NREGION_X; i++)
	{
		for (int j = 0; j < NREGION_Y; j++)
		{
			q = my_draw_sprite(q, i, j);
		}
	}

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	free(packet);

	graph_wait_vsync();

	return 0;
}

void save_image()
{
  read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, g_frame_data);

  char filename[128];
  
  sprintf(filename, "mass:" PREFIX "-tw%d-tbw%d-off%d-frac%d-rev%d-rot%d-shf%d-clmp%d-tri%d.bmp",
		TW, TBW, OFFSET_U, OFFSET_FRAC, REV, ROT, SHF, CLMP, TRI);

  if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, my_draw_clear_send) != 0)
  {
    printf("Failed to write frame data to USB\n");
  }
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	
	init_gs();

	make_texture();

	load_texture(&g_tex);

	render_test();

	save_image();

  SleepThread();

	return 0;
}