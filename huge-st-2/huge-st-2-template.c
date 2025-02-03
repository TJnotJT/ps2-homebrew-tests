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

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 512
#define WINDOW_X (2048 - (FRAME_WIDTH / 2))
#define WINDOW_Y (2048 - (FRAME_HEIGHT / 2))
#define TEXTURE_WIDTH 256
#define TEXTURE_HEIGHT 256
#define SQUARE_WIDTH 200
#define NORMAL_ST_VALUE 1
#define HUGE_ST_VALUE 1e38f

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

int vertices[4][2] = {
		{0, 0},
		{SQUARE_WIDTH, 0},
		{0, SQUARE_WIDTH},
		{SQUARE_WIDTH, SQUARE_WIDTH}
};

float coordinates[4][2] = {
		{0.00f, 0.00f},
		{NORMAL_ST_VALUE, 0.00f},
		{0.00f, NORMAL_ST_VALUE},
		{NORMAL_ST_VALUE, NORMAL_ST_VALUE}
};

unsigned char texture_data[TEXTURE_WIDTH][TEXTURE_HEIGHT][3] __attribute__((aligned(16)));

void make_texture()
{
	for (int x = 0; x < TEXTURE_WIDTH; x++)
	{
		for (int y = 0; y < TEXTURE_HEIGHT; y++)
		{
#if CHECKERBOARD_TYPE == 1
			if (x < (TEXTURE_WIDTH / 2) && y < (TEXTURE_HEIGHT / 2))
			{
				texture_data[x][y][0] = 0xFF;
				texture_data[x][y][1] = 0x00;
				texture_data[x][y][2] = 0x00;
			}
			else if (x >= (TEXTURE_WIDTH / 2) && y < (TEXTURE_HEIGHT / 2))
			{
				texture_data[x][y][0] = 0xFF;
				texture_data[x][y][1] = 0xFF;
				texture_data[x][y][2] = 0x00;
			}
			else if (x < (TEXTURE_WIDTH / 2) && y >= (TEXTURE_HEIGHT / 2))
			{
				texture_data[x][y][0] = 0x00;
				texture_data[x][y][1] = 0x00;
				texture_data[x][y][2] = 0xFF;
			}
			else // x >= (TEXTURE_WIDTH / 2) && y >= (TEXTURE_HEIGHT / 2)
			{
				texture_data[x][y][0] = 0x00;
				texture_data[x][y][1] = 0xFF;
				texture_data[x][y][2] = 0xFF;
			}
#elif CHECKERBOARD_TYPE == 0
			if (((x / 16) % 2 + (y / 16) % 2) == 1)
			{
				texture_data[x][y][0] = 0xFF;
				texture_data[x][y][1] = 0x00;
				texture_data[x][y][2] = 0xFF;
			}
			else
			{
				texture_data[x][y][0] = 0x00;
				texture_data[x][y][1] = 0xFF;
				texture_data[x][y][2] = 0x00;
			}
#endif
		}
	}
}

void init_gs(framebuffer_t *frame, zbuffer_t *z, texbuffer_t *texbuf)
{

	// Define a 32-bit 640x512 framebuffer.
	frame->width = FRAME_WIDTH;
	frame->height = FRAME_HEIGHT;
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
	texbuf->width = 1 << draw_log2(TEXTURE_WIDTH);
	texbuf->psm = GS_PSM_24;
	texbuf->address = graph_vram_allocate(TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_PSM_24, GRAPH_ALIGN_BLOCK);

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
	q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);

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

	q = draw_texture_transfer(q, texture_data, TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_PSM_24, texbuf->address, texbuf->width);
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

	texbuf->info.width = draw_log2(TEXTURE_WIDTH);
	texbuf->info.height = draw_log2(TEXTURE_HEIGHT);
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
	wrap.maxu = TEXTURE_WIDTH - 1;
	wrap.minv = 0;
	wrap.maxv = TEXTURE_HEIGHT - 1;

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
		q = draw_clear(q, 0, WINDOW_X, WINDOW_Y, frame->width, frame->height, 0xFF, 0xFF, 0xFF);
		q++;

		q = draw_enable_tests(q, 0, z);

		// Draw the triangles using triangle primitive type.
		// Use a 64-bit pointer to simplify adding data to the packet.
		dw = (u64 *)draw_prim_start(q, 0, &prim, &color);

		for (int rect = 0; rect < 5; rect++)
		{
			int offset_x, offset_y;
			float huge_s, huge_t;

			switch (rect)
			{
				case 0:
					offset_x = 0;
					offset_y = 0;
					huge_s = 0;
					huge_t = 0;
					break;
				case 1:
					offset_x = SQUARE_WIDTH + 10;
					offset_y = 0;
					huge_s = -HUGE_ST_VALUE;
					huge_t = -HUGE_ST_VALUE;
					break;
				case 2:
					offset_x = 2 * (SQUARE_WIDTH + 10);
					offset_y = 0;
					huge_s = HUGE_ST_VALUE;
					huge_t = -HUGE_ST_VALUE;
					break;
				case 3:
					offset_x = SQUARE_WIDTH + 10;
					offset_y = SQUARE_WIDTH + 10;
					huge_s = -HUGE_ST_VALUE;
					huge_t = HUGE_ST_VALUE;
					break;
				case 4:
					offset_x = 2 * (SQUARE_WIDTH + 10);
					offset_y = SQUARE_WIDTH + 10;
					huge_s = HUGE_ST_VALUE;
					huge_t = HUGE_ST_VALUE;
					break;
			}

			for (i = 0; i < 6; i++)
			{
				*dw++ = color.rgbaq;

				texel_t texel;
				texel.s = coordinates[points[i]][0];
				texel.t = coordinates[points[i]][1];
				if (rect > 0 && i == 0)
				{
					texel.s = huge_s;
					texel.t = huge_t;
				}
				*dw++ = texel.uv;

				xyz_t xyz;
				xyz.x = (vertices[points[i]][0] + WINDOW_X + offset_x) << 4;
				xyz.y = (vertices[points[i]][1] + WINDOW_Y + offset_y) << 4;
				xyz.z = 0;
				*dw++ = xyz.xyz;
			}
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
