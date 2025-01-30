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

#define NORMAL_ST_VALUE 10

#ifndef HUGE_S_VALUE
#define HUGE_S_VALUE 1e38
#endif

#ifndef HUGE_T_VALUE
#define HUGE_T_VALUE 1e38
#endif

#ifndef HUGE_ST
#define HUGE_ST 1
#endif

// REPEAT 0
// CLAMP 1
// REGION_CLAMP 2
// REGION_REPEAT 3

#ifndef CLAMP_WRAP_MODE_S
#define CLAMP_WRAP_MODE_S 0
#endif

#ifndef CLAMP_WRAP_MODE_T
#define CLAMP_WRAP_MODE_T 0
#endif

// Type 1 for 4 large squares with 4 different colors:
// 0xFF0000, 0xFFFF00,
// 0x0000FF, 0x00FFFF
// Type 2 for small squares with 2 colors
#ifndef CHECKERBOARD_TYPE
#define CHECKERBOARD_TYPE 1
#endif

int points[6] = {
		0, 1, 2,
		1, 2, 3,
};

int vertex_count = 24;

int vertices[4][2] = {
		{-100, -100},
		{100, -100},
		{-100, 100},
		{100, 100}
};

float coordinates[4][2] = {
		{0.00f, 0.00f},
		{NORMAL_ST_VALUE, 0.00f},
		{0.00f, NORMAL_ST_VALUE},
		{NORMAL_ST_VALUE, NORMAL_ST_VALUE}
};

unsigned char texture_data[256 * 256 * 3] __attribute__((aligned(16)));

VECTOR object_position = {0.00f, 0.00f, 0.00f, 1.00f};
VECTOR object_rotation = {0.00f, 0.00f, 0.00f, 1.00f};

VECTOR camera_position = {0.00f, 0.00f, 100.00f, 1.00f};
VECTOR camera_rotation = {0.00f, 0.00f, 0.00f, 1.00f};

void make_texture()
{
	for (int x = 0; x < 256; x++)
	{
		for (int y = 0; y < 256; y++)
		{
#if CHECKERBOARD_TYPE == 1
			if (x < 128 && y < 128)
			{
				texture_data[3 * x * 256 + 3 * y + 0] = 0xFF;
				texture_data[3 * x * 256 + 3 * y + 1] = 0x00;
				texture_data[3 * x * 256 + 3 * y + 2] = 0x00;
			}
			else if (x >= 128 && y < 128)
			{
				texture_data[3 * x * 256 + 3 * y + 0] = 0xFF;
				texture_data[3 * x * 256 + 3 * y + 1] = 0xFF;
				texture_data[3 * x * 256 + 3 * y + 2] = 0x00;
			}
			else if (x < 128 && y >= 128)
			{
				texture_data[3 * x * 256 + 3 * y + 0] = 0x00;
				texture_data[3 * x * 256 + 3 * y + 1] = 0x00;
				texture_data[3 * x * 256 + 3 * y + 2] = 0xFF;
			}
			else
			{
				texture_data[3 * x * 256 + 3 * y + 0] = 0x00;
				texture_data[3 * x * 256 + 3 * y + 1] = 0xFF;
				texture_data[3 * x * 256 + 3 * y + 2] = 0xFF;
			}
#elif CHECKERBOARD_TYPE == 0
			if (((x / 16) % 2 + (y / 16) % 2) == 1)
			{
				texture_data[3 * x * 256 + 3 * y + 0] = 0xFF;
				texture_data[3 * x * 256 + 3 * y + 1] = 0x00;
				texture_data[3 * x * 256 + 3 * y + 2] = 0xFF;
			}
			else
			{
				texture_data[3 * x * 256 + 3 * y + 0] = 0x00;
				texture_data[3 * x * 256 + 3 * y + 1] = 0xFF;
				texture_data[3 * x * 256 + 3 * y + 2] = 0x00;
			}
#endif
		}
	}
}

void init_gs(framebuffer_t *frame, zbuffer_t *z, texbuffer_t *texbuf)
{

	// Define a 32-bit 640x512 framebuffer.
	frame->width = 640;
	frame->height = 512;
	frame->mask = 0;
	frame->psm = GS_PSM_32;
	frame->address = graph_vram_allocate(frame->width, frame->height, frame->psm, GRAPH_ALIGN_PAGE);

	// Enable the zbuffer.
	z->enable = DRAW_ENABLE;
	z->mask = 0;
	z->method = ZTEST_METHOD_GREATER_EQUAL;
	z->zsm = GS_ZBUF_32;
	z->address = graph_vram_allocate(frame->width, frame->height, z->zsm, GRAPH_ALIGN_PAGE);

	// Allocate some vram for the texture buffer
	texbuf->width = 256;
	texbuf->psm = GS_PSM_24;
	texbuf->address = graph_vram_allocate(256, 256, GS_PSM_24, GRAPH_ALIGN_BLOCK);

	// Initialize the screen and tie the first framebuffer to the read circuits.
	graph_initialize(frame->address, frame->width, frame->height, frame->psm, 0, 0);

	graph_set_bgcolor(0, 0, 255);
}

void init_drawing_environment(framebuffer_t *frame, zbuffer_t *z)
{

	packet_t *packet = packet_init(20, PACKET_NORMAL);

	// This is our generic qword pointer.
	qword_t *q = packet->data;

	// This will setup a default drawing environment.
	q = draw_setup_environment(q, 0, frame, z);

	// Now reset the primitive origin to 2048-width/2,2048-height/2.
	q = draw_primitive_xyoffset(q, 0, (2048 - 320), (2048 - 256));

	// Finish setting up the environment.
	q = draw_finish(q);

	// Now send the packet, no need to wait since it's the first.
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
}

void load_texture(texbuffer_t *texbuf)
{

	packet_t *packet = packet_init(50, PACKET_NORMAL);

	qword_t *q;

	q = packet->data;

	q = draw_texture_transfer(q, texture_data, 256, 256, GS_PSM_24, texbuf->address, texbuf->width);
	q = draw_texture_flush(q);

	FlushCache(0); // Needed because we generate data in the EE.

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	dma_channel_wait(DMA_CHANNEL_GIF, -1);

	packet_free(packet);
}

void setup_texture(texbuffer_t *texbuf)
{

	packet_t *packet = packet_init(10, PACKET_NORMAL);

	qword_t *q = packet->data;

	// Using a texture involves setting up a lot of information.
	clutbuffer_t clut;

	lod_t lod;

	texwrap_t wrap;

	lod.calculation = LOD_USE_K;
	lod.max_level = 0;
	lod.mag_filter = LOD_MAG_NEAREST;
	lod.min_filter = LOD_MIN_NEAREST;
	lod.l = 0;
	lod.k = 0;

	texbuf->info.width = draw_log2(256);
	texbuf->info.height = draw_log2(256);
	texbuf->info.components = TEXTURE_COMPONENTS_RGB;
	texbuf->info.function = TEXTURE_FUNCTION_DECAL;

	clut.storage_mode = CLUT_STORAGE_MODE1;
	clut.start = 0;
	clut.psm = 0;
	clut.load_method = CLUT_NO_LOAD;
	clut.address = 0;

	wrap.horizontal = CLAMP_WRAP_MODE_S;
	wrap.vertical = CLAMP_WRAP_MODE_T;
	wrap.minu = 0;
	wrap.maxu = 255;
	wrap.minv = 0;
	wrap.maxv = 255;

	q = draw_texture_sampling(q, 0, &lod);
	q = draw_texturebuffer(q, 0, texbuf, &clut);
	q = draw_texture_wrapping(q, 0, &wrap);

	// Now send the packet, no need to wait since it's the first.
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
}

int render(framebuffer_t *frame, zbuffer_t *z)
{
	int i;
	int context = 0;

	packet_t *packets[2];
	packet_t *current;

	prim_t prim;
	color_t color;

	packets[0] = packet_init(100, PACKET_NORMAL);
	packets[1] = packet_init(100, PACKET_NORMAL);

	// Define the triangle primitive we want to use.
	prim.type = PRIM_TRIANGLE;
	prim.shading = PRIM_SHADE_GOURAUD;
	prim.mapping = DRAW_ENABLE;
	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_ENABLE;
	prim.antialiasing = DRAW_DISABLE;
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;

	color.r = 0x00;
	color.g = 0x00;
	color.b = 0x00;
	color.a = 0x80;
	color.q = 1.0f;

	// The main loop...
	for (;;)
	{
		qword_t *q;
		u64 *dw;

		current = packets[context];

		q = current->data;

		// Clear framebuffer but don't update zbuffer.
		q = draw_disable_tests(q, 0, z);
		q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 256.0f, frame->width, frame->height, 0xFF, 0xFF, 0xFF);
		q++;

		q = draw_enable_tests(q, 0, z);

		// Draw the triangles using triangle primitive type.
		// Use a 64-bit pointer to simplify adding data to the packet.
		dw = (u64 *)draw_prim_start(q, 0, &prim, &color);

		for (i = 0; i < 6; i++)
		{
			*dw++ = color.rgbaq;

			texel_t texel;
			texel.s = coordinates[points[i]][0];
			texel.t = coordinates[points[i]][1];
			if (HUGE_ST && i == 0)
			{
				texel.s = HUGE_S_VALUE;
				texel.t = HUGE_T_VALUE;
			}
			*dw++ = texel.uv;

			xyz_t xyz;
			xyz.x = (vertices[points[i]][0] + 2048) << 4;
			xyz.y = (vertices[points[i]][1] + 2048) << 4;
			xyz.z = 0;
			*dw++ = xyz.xyz;
		}

		// Check if we're in middle of a qword or not.
		if ((u32)dw % 16)
		{
			*dw++ = 0;
		}

		// Only 3 registers rgbaq/st/xyz were used (standard STQ reglist)
		q = draw_prim_end((qword_t *)dw, 3, DRAW_STQ_REGLIST);

		// Setup a finish event.
		q = draw_finish(q);

		// Now send our current dma chain.
		dma_wait_fast();
		dma_channel_send_normal(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);

		// Now switch our packets so we can process data while the DMAC is working.
		context ^= 1;

		// Wait for scene to finish drawing
		draw_wait_finish();

		graph_wait_vsync();
	}

	free(packets[0]);
	free(packets[1]);

	// End program.
	return 0;
}

int main(int argc, char *argv[])
{
	make_texture();

	// The buffers to be used.
	framebuffer_t frame;
	zbuffer_t z;
	texbuffer_t texbuf;

	// Init GIF dma channel.
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	// Init the GS, framebuffer, zbuffer, and texture buffer.
	init_gs(&frame, &z, &texbuf);

	// Init the drawing environment and framebuffer.
	init_drawing_environment(&frame, &z);

	// Load the texture into vram.
	load_texture(&texbuf);

	// Setup texture buffer
	setup_texture(&texbuf);

	// Render textured cube
	render(&frame, &z);

	// Sleep
	SleepThread();

	// End program.
	return 0;
}
