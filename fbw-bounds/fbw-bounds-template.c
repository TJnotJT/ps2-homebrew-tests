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
#define FRAME_HEIGHT 320
#define WINDOW_X (2048 - (FRAME_WIDTH / 2))
#define WINDOW_Y (2048 - (FRAME_HEIGHT / 2))
#define TEXTURE_WIDTH 640
#define TEXTURE_HEIGHT 320
#define TW (draw_log2(TEXTURE_WIDTH))
#define TH (draw_log2(TEXTURE_HEIGHT))

#ifndef FBW
#define FBW (FRAME_WIDTH / 64)
#endif

// 0 for line, 1 for triangle, 2 for sprite
#ifndef PRIM_TYPE
#define PRIM_TYPE 2
#endif

#ifndef USE_TEXTURE
#define USE_TEXTURE 1
#endif

extern unsigned char texture_data[640 * 320 * 3];

// Texture data with numbers for each page (64 x 32 pixels)
#include "texture.c"

// unsigned char texture_data[256 * 256 * 3] __attribute__((aligned(16)));

// void make_texture()
// {
// 	for (int x = 0; x < 256; x++)
// 	{
// 		for (int y = 0; y < 256; y++)
// 		{
// 			if (x < 128 && y < 128)
// 			{
// 				texture_data[3 * x * 256 + 3 * y + 0] = 0xFF;
// 				texture_data[3 * x * 256 + 3 * y + 1] = 0x00;
// 				texture_data[3 * x * 256 + 3 * y + 2] = 0x00;
// 			}
// 			else if (x >= 128 && y < 128)
// 			{
// 				texture_data[3 * x * 256 + 3 * y + 0] = 0xFF;
// 				texture_data[3 * x * 256 + 3 * y + 1] = 0xFF;
// 				texture_data[3 * x * 256 + 3 * y + 2] = 0x00;
// 			}
// 			else if (x < 128 && y >= 128)
// 			{
// 				texture_data[3 * x * 256 + 3 * y + 0] = 0x00;
// 				texture_data[3 * x * 256 + 3 * y + 1] = 0x00;
// 				texture_data[3 * x * 256 + 3 * y + 2] = 0xFF;
// 			}
// 			else
// 			{
// 				texture_data[3 * x * 256 + 3 * y + 0] = 0x00;
// 				texture_data[3 * x * 256 + 3 * y + 1] = 0xFF;
// 				texture_data[3 * x * 256 + 3 * y + 2] = 0xFF;
// 			}
// 		}
// 	}
// }

u32 bit_cast_float(float f)
{
	u32 *i = (u32*)&f;
	return *i;
}

void init_gs(framebuffer_t *frame, zbuffer_t *z, texbuffer_t *texbuf)
{

	frame->width = FRAME_WIDTH;
	frame->height = FRAME_HEIGHT;
	frame->mask = 0;
	frame->psm = GS_PSM_32;
	frame->address = graph_vram_allocate(frame->width, frame->height, frame->psm, GRAPH_ALIGN_PAGE);

	// Enable the zbuffer.
	z->enable = DRAW_ENABLE;
	z->mask = 0;
	z->method = ZTEST_METHOD_ALLPASS;
	z->zsm = GS_ZBUF_32;
	z->address = graph_vram_allocate(frame->width, frame->height, z->zsm, GRAPH_ALIGN_PAGE);

	// Allocate some vram for the texture buffer
	texbuf->width = 1 << TW;
	texbuf->psm = GS_PSM_24;
	texbuf->address = graph_vram_allocate(1 << TW, 1 << TH, GS_PSM_24, GRAPH_ALIGN_BLOCK);

	// Initialize the screen and tie the first framebuffer to the read circuits.
	graph_initialize(frame->address, frame->width, frame->height, frame->psm, 0, 0);

	graph_set_bgcolor(0, 0, 0);
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

	texbuf->info.width = TW;
	texbuf->info.height = TH;
	texbuf->info.components = TEXTURE_COMPONENTS_RGB;
	texbuf->info.function = TEXTURE_FUNCTION_DECAL;

	clut.storage_mode = CLUT_STORAGE_MODE1;
	clut.start = 0;
	clut.psm = 0;
	clut.load_method = CLUT_NO_LOAD;
	clut.address = 0;

	wrap.horizontal = 0; // Repeat
	wrap.vertical = 0; // Repeat
	wrap.minu = 0;
	wrap.maxu = 0;
	wrap.minv = 0;
	wrap.maxv = 0;

	q = draw_texture_sampling(q, 0, &lod);
	q = draw_texturebuffer(q, 0, texbuf, &clut);
	q = draw_texture_wrapping(q, 0, &wrap);

	// Now send the packet, no need to wait since it's the first.
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
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

int render(framebuffer_t *frame, zbuffer_t *z)
{
	int context = 0;

	packet_t *packets[2];
	packet_t *current;

	prim_t prim;
	color_t color;

	packets[0] = packet_init(100, PACKET_NORMAL);
	packets[1] = packet_init(100, PACKET_NORMAL);

	// Define the triangle primitive we want to use.

#if PRIM_TYPE == 0
	prim.type = PRIM_LINE;
#elif PRIM_TYPE == 1
	prim.type = PRIM_TRIANGLE;
#elif PRIM_TYPE == 2
	prim.type = PRIM_SPRITE;
#endif

	prim.shading = PRIM_SHADE_FLAT;

#if USE_TEXTURE == 1
	prim.mapping = DRAW_ENABLE;
#else
	prim.mapping = DRAW_DISABLE;
#endif

	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_ENABLE;
	prim.antialiasing = DRAW_DISABLE;
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;

	color.r = 0xFF;
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
		q = draw_clear(q, 0, (float)WINDOW_X, (float)WINDOW_Y, frame->width, frame->height, 0xFF, 0xFF, 0xFF);
		q++;

		q = draw_enable_tests(q, 0, z);

		PACK_GIFTAG(q, GIF_SET_TAG(2, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(frame->address >> 11, FBW, frame->psm, frame->mask), GS_REG_FRAME_1);
		q++;
		PACK_GIFTAG(q, GS_SET_SCISSOR(0, FRAME_WIDTH - 1, 0, FRAME_HEIGHT - 1), GS_REG_SCISSOR_1);
		q++;

		// Use a 64-bit pointer to simplify adding data to the packet.
		dw = (u64*)draw_prim_start(q, 0, &prim, &color);

		*dw++ = GIF_SET_RGBAQ(0xFF, 0, 0, 0x80, bit_cast_float(1.0f));
		*dw++ = GIF_SET_ST(bit_cast_float(0.0f), bit_cast_float(0.0f));
		*dw++ = GIF_SET_XYZ(WINDOW_X << 4, WINDOW_Y << 4, 0);
		
		*dw++ = GIF_SET_RGBAQ(0, 0, 0xFF, 0x80, bit_cast_float(1.0f));
		*dw++ = GIF_SET_ST(
			bit_cast_float((float) TEXTURE_WIDTH / (1 << TW)),
			bit_cast_float((float) TEXTURE_HEIGHT / (1 << TH))
		);
		*dw++ = GIF_SET_XYZ((WINDOW_X + FRAME_WIDTH) << 4, (WINDOW_Y + FRAME_HEIGHT) << 4, 0);
		
// For triangle
#if PRIM_TYPE == 1
		*dw++ = GIF_SET_RGBAQ(0, 0xFF, 0, 0x80, bit_cast_float(1.0f));
		*dw++ = GIF_SET_ST(bit_cast_float(0.0f), bit_cast_float((float) TEXTURE_HEIGHT / (1 << TH)));
		*dw++ = GIF_SET_XYZ(WINDOW_X << 4, (WINDOW_Y + FRAME_HEIGHT) << 4, 0);
#endif
		// Check if we're in middle of a qword or not.
		if ((u32)dw % 16)
		{
			*dw++ = 0;
		}

		// Only 3 registers rgbaq/st/xyz were used (standard STQ reglist)
		q = draw_prim_end((qword_t*)dw, 3, DRAW_STQ_REGLIST);

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
	// make_texture();

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

	render(&frame, &z);

	// Sleep
	SleepThread();

	// End program.
	return 0;
}
