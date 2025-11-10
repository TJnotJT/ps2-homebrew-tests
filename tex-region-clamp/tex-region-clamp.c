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

#define FRAME_WIDTH 128
#define FRAME_HEIGHT 128
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)
#define TEX_BUFF_WIDTH 64
#define TEX_WIDTH 64
#define TEX_HEIGHT 32
#define TEST_REGION_SIZE 16
#define TEST_REGIONS_X 5
#define TEST_REGIONS_Y 5
#define TEST_REGIONS (TEST_REGIONS_X * TEST_REGIONS_Y)

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z; // Z buffer
texbuffer_t g_tex; // Texture buffer
texbuffer_t g_tex_pad; // Padded Texture buffer
clutbuffer_t g_clut;
lod_t g_lod;
texwrap_t g_wrap;

u8 g_frame_data[FRAME_WIDTH * FRAME_HEIGHT * 4] __attribute__((aligned(64)));

u8 texture_data[TEX_WIDTH * TEX_HEIGHT * 4] __attribute__((aligned(64)));
u8 texture_data_pad[TEX_WIDTH * TEX_HEIGHT * 4] __attribute__((aligned(64)));

// Checkerboard texture
void make_texture()
{
	for (int i = 0; i < TEX_HEIGHT; i++)
	{
		for (int j = 0; j < TEX_WIDTH; j++)
		{
			int intensity = (255 * ((i % (TEX_HEIGHT / 2)) + (j % (TEX_WIDTH / 2)))) / ((TEX_WIDTH / 2) + (TEX_HEIGHT / 2) - 2);
			if (i < TEX_HEIGHT / 2 && j < TEX_WIDTH / 2)
			{
				texture_data[(i * TEX_WIDTH + j) * 4 + 0] = intensity; // R
				texture_data[(i * TEX_WIDTH + j) * 4 + 1] = 0; // G
				texture_data[(i * TEX_WIDTH + j) * 4 + 2] = 0; // B
				texture_data[(i * TEX_WIDTH + j) * 4 + 3] = 0xFF; // A
			}
			else if (i < TEX_HEIGHT / 2 && j >= TEX_WIDTH / 2)
			{
				texture_data[(i * TEX_WIDTH + j) * 4 + 0] = 0; // R
				texture_data[(i * TEX_WIDTH + j) * 4 + 1] = intensity; // G
				texture_data[(i * TEX_WIDTH + j) * 4 + 2] = 0; // B
				texture_data[(i * TEX_WIDTH + j) * 4 + 3] = 0xFF; // A
			}
			else if (i >= TEX_HEIGHT / 2 && j < TEX_WIDTH / 2)
			{
				texture_data[(i * TEX_WIDTH + j) * 4 + 0] = 0; // R
				texture_data[(i * TEX_WIDTH + j) * 4 + 1] = 0; // G
				texture_data[(i * TEX_WIDTH + j) * 4 + 2] = intensity; // B
				texture_data[(i * TEX_WIDTH + j) * 4 + 3] = 0xFF; // A
			}
			else if (i >= TEX_HEIGHT / 2 && j >= TEX_WIDTH / 2)
			{
				texture_data[(i * TEX_WIDTH + j) * 4 + 0] = intensity; // R
				texture_data[(i * TEX_WIDTH + j) * 4 + 1] = 0; // G
				texture_data[(i * TEX_WIDTH + j) * 4 + 2] = intensity; // B
				texture_data[(i * TEX_WIDTH + j) * 4 + 3] = 0xFF; // A
			}
		}
	}

	for (int i = 0; i < TEX_HEIGHT; i++)
	{
		for (int j = 0; j < TEX_WIDTH; j++)
		{
			texture_data_pad[(i * TEX_WIDTH + j) * 4 + 0] = 0xFF; // R
			texture_data_pad[(i * TEX_WIDTH + j) * 4 + 1] = 0xFF; // G
			texture_data_pad[(i * TEX_WIDTH + j) * 4 + 2] = 0xFF; // B
			texture_data_pad[(i * TEX_WIDTH + j) * 4 + 3] = 0xFF; // A
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
	g_z.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_z.zsm, GRAPH_ALIGN_PAGE);

	g_tex.width = TEX_BUFF_WIDTH;
	g_tex.psm = GS_PSM_32;
	g_tex.address = graph_vram_allocate(g_tex.width, TEX_HEIGHT, GS_PSM_32, GRAPH_ALIGN_BLOCK);

	g_tex_pad.width = TEX_BUFF_WIDTH;
	g_tex_pad.psm = GS_PSM_32;
	g_tex_pad.address = graph_vram_allocate(g_tex_pad.width, TEX_HEIGHT, GS_PSM_32, GRAPH_ALIGN_BLOCK);

	g_tex_pad.info.width = g_tex.info.width = draw_log2(TEX_WIDTH);
	g_tex_pad.info.height = g_tex.info.height = draw_log2(TEX_HEIGHT);
	g_tex_pad.info.function = g_tex.info.function = TEXTURE_FUNCTION_DECAL;
	g_tex_pad.info.components = g_tex.info.components = TEXTURE_COMPONENTS_RGBA;

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

	g_wrap.horizontal = WRAP_REGION_CLAMP;
	g_wrap.vertical = WRAP_REGION_CLAMP;
	g_wrap.minu = 0;
	g_wrap.maxu = 0 ;
	g_wrap.minv = 0;
	g_wrap.maxv = 0;

	graph_initialize_custom();
}

qword_t* setup_texture(qword_t* q, int buff_width, int tex_width, int tex_height, int minu, int maxu, int minv, int maxv)
{
	g_wrap.minu = minu;
	g_wrap.maxu = maxu;
	g_wrap.minv = minv;
	g_wrap.maxv = maxv;

	texbuffer_t buf = g_tex;
	buf.width = buff_width;
	buf.info.width = draw_log2(tex_width);
	buf.info.height = draw_log2(tex_height);

	q = draw_texture_sampling(q, 0, &g_lod);
	q = draw_texturebuffer(q, 0, &buf, &g_clut);
	q = draw_texture_wrapping(q, 0, &g_wrap);

	return q;
}

void load_texture(texbuffer_t *texbuf, u8* data)
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);

	qword_t* q = packet;

	q = draw_texture_transfer(q, data, 1 << texbuf->info.width, 1 << texbuf->info.height,
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
	bg_color.a = 0x80;
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

void send_dummy_eop()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8);
	qword_t* q = packet;
	
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	free(packet);
}

void send_mem_clear(u32 value)
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 128);
	qword_t* q = packet;

	PACK_GIFTAG(q, GIF_SET_TAG(7, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_FRAME(0, 16, GS_PSM_32, 0), GS_REG_FRAME);
	q++;

	PACK_GIFTAG(q, GS_SET_TEST(0, ATEST_METHOD_ALLPASS, 0x00, ATEST_KEEP_ALL,
							   0, 0,
							   1, ZTEST_METHOD_ALLPASS), GS_REG_TEST);
	q++;

	PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0), GS_REG_PRIM);
	q++;

	PACK_GIFTAG(q, GS_SET_RGBAQ(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF, 0x3F800000), GS_REG_RGBAQ);
	q++;

	PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET);
	q++;

	PACK_GIFTAG(q, GIF_SET_XYZF(0, 0, 0, 0), GIF_REG_XYZ2);
	q++;

	PACK_GIFTAG(q, GIF_SET_XYZF(0, 0, 0, 0), GIF_REG_XYZ2);
	q++;

	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	free(packet);
}

qword_t* draw_sprite(qword_t* q, int region)
{
	const int x = WINDOW_X + TEST_REGION_SIZE * (region % TEST_REGIONS_X);
	const int y = WINDOW_Y + TEST_REGION_SIZE * (region / TEST_REGIONS_X);

	int minu = (region % TEST_REGIONS_X) * (TEX_WIDTH / 4);
	int maxu = minu + TEX_WIDTH;
	int minv = ((region / TEST_REGIONS_X) % TEST_REGIONS_Y) * (TEX_HEIGHT / 4);
	int maxv = minv + TEX_HEIGHT;

	q = setup_texture(q, TEX_BUFF_WIDTH, TEX_WIDTH, TEX_HEIGHT, minu, maxu, minv, maxv);

	PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0), GS_REG_PRIM);
	q++;

	u32 x0 = (x + 1) << 4;
	u32 x1 = (x + TEST_REGION_SIZE - 1) << 4;
	u32 y0 = (y + 1) << 4;
	u32 y1 = (y + TEST_REGION_SIZE - 1) << 4;
	u32 u0 = 0;
	u32 u1 = TEX_WIDTH << 4;
	u32 v0 = 0 << 4;
	u32 v1 = TEX_HEIGHT << 4;
	u32 z0 = 0;
	u32 z1 = 0;

	PACK_GIFTAG(q, GIF_SET_UV(u0, v0), GIF_REG_UV);
	q++;

	PACK_GIFTAG(q, GIF_SET_XYZ(x0, y0, z0), GIF_REG_XYZ2);
	q++;

	PACK_GIFTAG(q, GIF_SET_UV(u1, v1), GIF_REG_UV);
	q++;

	PACK_GIFTAG(q, GIF_SET_XYZ(x1, y1, z1), GIF_REG_XYZ2);
	q++;

	return q;
}

int render_test()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q;

	// *GS_REG_CSR = 2; // Clear previous finish event.

  // Set up the drawing environment
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame, &g_z);
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
	q = my_draw_disable_tests(q, 0);
	q = my_draw_clear(q, 0);
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	// Draw a textured sprite
	q = packet;

	for (int region = 0; region < TEST_REGIONS; region++)
	{
		q = draw_sprite(q, region);
	}
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	free(packet);

	graph_wait_vsync();

	return 0;
}

void save_image()
{
  read_framebuffer(g_frame.address, FRAME_WIDTH / 64, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, g_frame_data);

  char filename[64];
  
  sprintf(filename, "mass:%s", "tex-region-clamp.bmp");

  if (write_bmp_to_usb(filename, g_frame_data, FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, my_draw_clear_send) != 0)
  {
    printf("Failed to write frame data to USB\n");
  }
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	
	init_gs();

	send_dummy_eop();

	send_mem_clear(0x00000000); // Clear to black

	make_texture();

	load_texture(&g_tex, texture_data);
	load_texture(&g_tex_pad, texture_data_pad);

	render_test();

	save_image();

  SleepThread();

	return 0;
}