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

#ifndef USE_Z
#define USE_Z 0
#endif

#ifndef USE_DATE
#define USE_DATE 0
#endif

#ifndef USE_TEX
#define USE_TEX 0
#endif

#ifndef TOTAL_TRIANGLES
#define TOTAL_TRIANGLES 4096
#endif

#ifndef BATCH_TRIANGLES
#define BATCH_TRIANGLES 256
#endif

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 256
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)

#define TEX_BUF_WIDTH 256
#define TEX_WIDTH 256
#define TEX_HEIGHT 256

#define CTX_MAIN 0
#define CTX_CLEAR 1
#define N_CTX 2

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z; // Z buffer
texbuffer_t g_tex; // Texture buffer

// Drawing context
clutbuffer_t g_clut;
lod_t g_lod;
texwrap_t g_wrap;
blend_t g_blend_std[N_CTX];
blend_t g_blend_alt[N_CTX];
atest_t g_atest[N_CTX];
dtest_t g_dtest[N_CTX];
ztest_t g_ztest[N_CTX];

u8 g_frame_data[FRAME_WIDTH * FRAME_HEIGHT * 4] __attribute__((aligned(64)));
u8 g_texture_data[TEX_WIDTH * TEX_HEIGHT * 4] __attribute__((aligned(64)));

int max(int a, int b)
{
	return (a > b) ? a : b;
}

int min(int a, int b)
{
	return (a < b) ? a : b;
}

// Make a checkerboard texture.
void make_texture()
{
	for (int y = 0; y < TEX_HEIGHT; y++)
	{
		for (int x = 0; x < TEX_WIDTH; x++)
		{
			int i = (y * TEX_WIDTH + x) * 4;
			if (((x / 16) + (y / 16)) & 1)
			{
				g_texture_data[i + 0] = 0x40;
				g_texture_data[i + 1] = 0x40;
				g_texture_data[i + 2] = 0x40;
				g_texture_data[i + 3] = 0x80;
			}
			else
			{
				g_texture_data[i + 0] = 0x80;
				g_texture_data[i + 1] = 0x80;
				g_texture_data[i + 2] = 0x80;
				g_texture_data[i + 3] = 0x80;
			}
		}
	}

	FlushCache(0); // Needed because we generate data in the EE.
}

void load_texture(texbuffer_t* texbuf)
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);

	qword_t* q = packet;

	q = draw_texture_transfer(q, g_texture_data, 1 << texbuf->info.width, 1 << texbuf->info.height,
		texbuf->psm, texbuf->address, texbuf->width);
	q = draw_texture_flush(q);

	FlushCache(0); // Needed because we generate data in the EE.

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, -1);

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
	g_tex.info.function = TEXTURE_FUNCTION_MODULATE;
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

	// Main context settings
	g_blend_std[CTX_MAIN].color1 = BLEND_COLOR_SOURCE;
	g_blend_std[CTX_MAIN].color2 = BLEND_COLOR_DEST;
	g_blend_std[CTX_MAIN].color3 = BLEND_COLOR_DEST;
	g_blend_std[CTX_MAIN].alpha = BLEND_ALPHA_SOURCE;
	g_blend_std[CTX_MAIN].fixed_alpha = 0x80;

	g_blend_alt[CTX_MAIN].color1 = BLEND_COLOR_DEST;
	g_blend_alt[CTX_MAIN].color2 = BLEND_COLOR_SOURCE;
	g_blend_alt[CTX_MAIN].color3 = BLEND_COLOR_SOURCE;
	g_blend_alt[CTX_MAIN].alpha = BLEND_ALPHA_SOURCE;
	g_blend_alt[CTX_MAIN].fixed_alpha = 0x80;

	g_atest[CTX_MAIN].compval = 0;
	g_atest[CTX_MAIN].enable = DRAW_DISABLE;
	g_atest[CTX_MAIN].keep = ATEST_KEEP_ALL;
	g_atest[CTX_MAIN].method = ATEST_METHOD_ALLPASS;

	g_dtest[CTX_MAIN].enable = USE_DATE ? DRAW_ENABLE : DRAW_DISABLE;
	g_dtest[CTX_MAIN].pass = DTEST_METHOD_PASS_ONE;

	g_ztest[CTX_MAIN].enable = DRAW_ENABLE;
	g_ztest[CTX_MAIN].method = USE_Z ? ZTEST_METHOD_GREATER_EQUAL : ZTEST_METHOD_ALLPASS;

	// Clear context settings
	g_blend_alt[CTX_CLEAR].color1 = g_blend_std[CTX_CLEAR].color1 = BLEND_COLOR_ZERO;
	g_blend_alt[CTX_CLEAR].color2 = g_blend_std[CTX_CLEAR].color2 = BLEND_COLOR_ZERO;
	g_blend_alt[CTX_CLEAR].color3 = g_blend_std[CTX_CLEAR].color3 = BLEND_COLOR_SOURCE;
	g_blend_alt[CTX_CLEAR].alpha = g_blend_std[CTX_CLEAR].alpha = BLEND_ALPHA_FIXED;
	g_blend_alt[CTX_CLEAR].fixed_alpha = g_blend_std[CTX_CLEAR].fixed_alpha = 0x80;

	g_atest[CTX_CLEAR].compval = 0;
	g_atest[CTX_CLEAR].enable = DRAW_DISABLE;
	g_atest[CTX_CLEAR].keep = ATEST_KEEP_ALL;
	g_atest[CTX_CLEAR].method = ATEST_METHOD_ALLPASS;

	g_dtest[CTX_CLEAR].enable = DRAW_DISABLE;
	g_dtest[CTX_CLEAR].pass = DTEST_METHOD_PASS_ONE;

	g_ztest[CTX_CLEAR].enable = DRAW_ENABLE;
	g_ztest[CTX_CLEAR].method = ZTEST_METHOD_ALLPASS;

	graph_initialize_custom();
}

qword_t* my_draw_clear(qword_t* q, int context, unsigned rgb)
{
	color_t bg_color;
	bg_color.r = rgb & 0xFF;
	bg_color.g = (rgb >> 8) & 0xFF;
	bg_color.b = (rgb >> 16) & 0xFF;
	bg_color.a = 0xFF;
	bg_color.q = 1.0f;

	PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GIF_SET_PRIM(PRIM_SPRITE, 0, 0, 0, 0, 0, 0, context, 0), GIF_REG_PRIM);
	q++;
	PACK_GIFTAG(q, bg_color.rgbaq, GIF_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(WINDOW_X << 4, WINDOW_Y << 4, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ((WINDOW_X + FRAME_WIDTH) << 4, (WINDOW_Y + FRAME_HEIGHT) << 4, 0), GIF_REG_XYZ2);
	q++;
	return q;
}

qword_t* my_draw_triangles(qword_t* q, int context, int n)
{
	PACK_GIFTAG(q, GIF_SET_TAG(1 + 9 * n, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_TRIANGLE, 0, USE_TEX, 0, 1, 0, 1, context, 0), GIF_REG_PRIM);
  q++;

	for (int i = 0; i < 3 * n; i++)
	{
		const int x = (WINDOW_X << 4) + rand() % (FRAME_WIDTH << 4);
		const int y = (WINDOW_Y << 4) + rand() % (FRAME_HEIGHT << 4);
		const int z = rand() % (1 << 24);

		const int u = rand() % (TEX_WIDTH << 4);
		const int v = rand() % (TEX_HEIGHT << 4);

		color_t color;
		color.r = rand() % 256;
		color.g = rand() % 256;
		color.b = rand() % 256;
		color.a = rand() % 256;
		color.q = 1.0f;

		PACK_GIFTAG(q, color.rgbaq, GS_REG_RGBAQ);
		q++;

		PACK_GIFTAG(q, GS_SET_UV(u, v), GS_REG_UV);
		q++;

		PACK_GIFTAG(q, GS_SET_XYZ(x, y, z), GS_REG_XYZ2);
		q++;
	}

	return q;
}

void setup_drawing_env()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);

	// Send a dummy EOP packet
	qword_t* q = packet;
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

  // Set up the drawing environment
	q = packet;
	for (int i = 0; i < N_CTX; i++)
	{
		q = draw_setup_environment(q, i, &g_frame, &g_z);
		q = draw_texture_sampling(q, i, &g_lod);
		q = draw_texturebuffer(q, i, &g_tex, &g_clut);
		q = draw_texture_wrapping(q, i, &g_wrap);
		q = draw_alpha_blending(q, i, &g_blend_std[i]);
		q = draw_primitive_xyoffset(q, i, (float)WINDOW_X, (float)WINDOW_Y);
		q = draw_pixel_test(q, i, &g_atest[i], &g_dtest[i], &g_ztest[i]);
	}
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	free(packet);
}

int render_test()
{
	srand(123);

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 65536);
	qword_t* q;

	q = packet;
	q = my_draw_clear(q, CTX_CLEAR, 0);

	int i = 0;
	int remaining_triangles = TOTAL_TRIANGLES;
	while (remaining_triangles > 0)
	{
		// Flip blend equations to force PCSX2 to split draws.
		blend_t blend = (i & 1) ? g_blend_alt[CTX_MAIN] : g_blend_std[CTX_MAIN];
		dtest_t dtest = g_dtest[CTX_MAIN];
		ztest_t ztest = g_ztest[CTX_MAIN];

		// Flip DATE on/off so that RT alpha gets changed.
		dtest.enable = USE_DATE ? !!(i & 2) : 0;

		q = draw_alpha_blending(q, CTX_MAIN, &blend);
		q = draw_pixel_test(q, CTX_MAIN, &g_atest[CTX_MAIN], &dtest, &ztest);
		q = my_draw_triangles(q, CTX_MAIN, min(BATCH_TRIANGLES, remaining_triangles));

		remaining_triangles -= min(BATCH_TRIANGLES, remaining_triangles);
		i++;
	}

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
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

	setup_drawing_env();

	while(1)
	{
		render_test();
	}

	SleepThread();

	return 0;
}
