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
#include <math3d.h>

#include <packet.h>

#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <gs_privileged.h>
#include <gs_gp.h>

#include <dma.h>

#include <graph.h>

#include <draw.h>
#include <draw3d.h>

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 256
#define WINDOW_X (2048 - (FRAME_WIDTH / 2))
#define WINDOW_Y (2048 - (FRAME_HEIGHT / 2))

framebuffer_t g_frame[2]; // Frame buffer for even/odd lines
zbuffer_t g_z; // Z buffer
texbuffer_t g_texbuf; // Texture buffer

int graph_initialize_custom()
{
	int mode = graph_get_region();

	graph_set_mode(GRAPH_MODE_INTERLACED, mode, GRAPH_MODE_FRAME, GRAPH_ENABLE);

	graph_set_screen(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

	graph_set_bgcolor(0, 0, 0);

	graph_set_framebuffer(0, g_frame[0].address, g_frame[0].width, g_frame[0].psm, 0, 0);

	graph_set_output(1, 0, 1, 0, 1, 0xFF);

	return 0;

}

void init_gs()
{
	for (int i = 0; i < 2; i++)
	{
		g_frame[i].width = FRAME_WIDTH;
		g_frame[i].height = FRAME_HEIGHT / 2; // Half height for FRAME interlaced mode.
		g_frame[i].mask = 0;
		g_frame[i].psm = GS_PSM_32;
		g_frame[i].address = graph_vram_allocate(g_frame[i].width, g_frame[i].height, g_frame[i].psm, GRAPH_ALIGN_PAGE);
	}

	g_z.enable = DRAW_DISABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = GS_ZBUF_32;
	g_z.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_z.zsm, GRAPH_ALIGN_PAGE);

	g_texbuf.width = 256;
	g_texbuf.psm = GS_PSM_24;
	g_texbuf.address = graph_vram_allocate(256, 256, GS_PSM_24, GRAPH_ALIGN_BLOCK);

	graph_initialize_custom();
}

void init_drawing_environment()
{

	packet_t *packet = packet_init(20, PACKET_NORMAL);

	qword_t *q = packet->data;

	q = draw_setup_environment(q, 0, &g_frame[0], &g_z);

	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);

	// Finish setting up the environment.
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
}

void load_texture()
{

	packet_t *packet = packet_init(50, PACKET_NORMAL);

	qword_t *q;

	q = packet->data;

	q = draw_texture_flush(q);

	FlushCache(0); // Needed because we generate data in the EE.

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	dma_channel_wait(DMA_CHANNEL_GIF, -1);

	packet_free(packet);
}

void setup_texture()
{

	packet_t *packet = packet_init(10, PACKET_NORMAL);

	qword_t *q = packet->data;

	clutbuffer_t clut;

	lod_t lod;

	texwrap_t wrap;

	lod.calculation = LOD_USE_K;
	lod.max_level = 0;
	lod.mag_filter = LOD_MAG_NEAREST;
	lod.min_filter = LOD_MIN_NEAREST;
	lod.l = 0;
	lod.k = 0;

	g_texbuf.info.width = draw_log2(256);
	g_texbuf.info.height = draw_log2(256);
	g_texbuf.info.components = TEXTURE_COMPONENTS_RGB;
	g_texbuf.info.function = TEXTURE_FUNCTION_DECAL;

	clut.storage_mode = CLUT_STORAGE_MODE1;
	clut.start = 0;
	clut.psm = 0;
	clut.load_method = CLUT_NO_LOAD;
	clut.address = 0;

	wrap.horizontal = 0;
	wrap.vertical = 0;
	wrap.minu = 0;
	wrap.maxu = 255;
	wrap.minv = 0;
	wrap.maxv = 255;

	q = draw_texture_sampling(q, 0, &lod);
	q = draw_texturebuffer(q, 0, &g_texbuf, &clut);
	q = draw_texture_wrapping(q, 0, &wrap);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
}

int render()
{
	int context = 0;

	packet_t *packets[2];
	packet_t *current;

	prim_t prim;
	color_t color[2];

	color[0].r = 0xFF;
	color[0].g = 0x00;
	color[0].b = 0x00;
	color[0].a = 0x80;
	color[0].q = 1.0f;

	color[1].r = 0xFF;
	color[1].g = 0xFF;
	color[1].b = 0xFF;
	color[1].a = 0x80;
	color[1].q = 1.0f;

	packets[0] = packet_init(100, PACKET_NORMAL);
	packets[1] = packet_init(100, PACKET_NORMAL);

	prim.type = PRIM_SPRITE;
	prim.shading = PRIM_SHADE_FLAT;
	prim.mapping = DRAW_DISABLE;
	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_DISABLE;
	prim.antialiasing = DRAW_DISABLE;
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;

	for (;;)
	{
		qword_t *q;
		u64 *dw;

		graph_set_framebuffer(0, g_frame[context^1].address, g_frame[context^1].width, g_frame[context^1].psm, 0, 0);

		current = packets[context];

		q = current->data;

		q = draw_framebuffer(q, 0, &g_frame[context]);

		dw = (u64*)draw_prim_start(q, 0, &prim, &color[context]);

		xyz_t xyz;
		xyz.x = WINDOW_X << 4;
		xyz.y = WINDOW_Y << 4;
		xyz.z = 0;
		*dw++ = xyz.xyz;

		*dw++ = color[context].rgbaq;

		xyz.x = (WINDOW_X + FRAME_WIDTH) << 4;
		xyz.y = (WINDOW_Y + FRAME_HEIGHT / 2) << 4;
		xyz.z = 0;
		*dw++ = xyz.xyz;

		if ((u32)dw % 16)
		{
			*dw++ = 0;
		}

		q = draw_prim_end((qword_t *)dw, 3, GIF_REG_XYZ2 | (GIF_REG_RGBAQ << 4) | (GIF_REG_XYZ2 << 8));

		q = draw_finish(q);

		dma_wait_fast();
		dma_channel_send_normal(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);

		context ^= 1;

		draw_wait_finish();

		printf("%d %d\n", context, !!(*GS_REG_CSR & (1 << 13)));

		graph_wait_vsync();
	}

	free(packets[0]);
	free(packets[1]);

	return 0;
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	init_gs();

	init_drawing_environment();

	load_texture();

	setup_texture();

	render();

	SleepThread();

	return 0;
}
