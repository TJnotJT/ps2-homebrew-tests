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
#include <stdbool.h>
#include <malloc.h>
#include <tamtypes.h>
#include <math3d.h>
#include <limits.h>

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

#define TEX_SIZE 256
#define TEX_BORDER_SIZE 16
#define TEX_CHECKER_SIZE 4
#define TEX_COLOR_1 ((unsigned int)0x8FF00000)
#define TEX_COLOR_2 ((unsigned int)0x80FFFF00)
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 224
#define OFX FRAME_WIDTH
#define OFY FRAME_HEIGHT
#define DRAW_START_X 100
#define DRAW_START_Y 100
#define TRIANGLE_WIDTH 16
#define TRIANGLE_HEIGHT 16
#define DRAW_START_U 32
#define DRAW_START_V 32
#define TRIANGLE_SIZE_U 16
#define TRIANGLE_SIZE_V 16
#define N_TRIANGLES_X 4
#define N_TRIANGLES_Y 4
#define N_STRIPS N_TRIANGLES_Y
#define N_STRIP_INDICES (2 * N_TRIANGLES_X + 2)
#define N_VERTICES_X (N_TRIANGLES_X + 1)
#define N_VERTICES_Y (N_TRIANGLES_Y + 1)
#define HUGE_VALUE_S 1e38
#define HUGE_VALUE_T -1e38
#define HUGE_INDEX_X 2
#define HUGE_INDEX_Y 2

#ifndef HUGE_ST
#define HUGE_ST 0
#endif

#define vX 0
#define vY 1
#define vZ 2
#define vS 3
#define vT 4
#define vQ 5

// Holds the texture data
unsigned char TEXTURE_DATA[TEX_SIZE][TEX_SIZE][4] __attribute__((aligned(16)));

float VERTICES[N_VERTICES_X][N_VERTICES_Y][6]; // X, Y, Z, S, T, Q
unsigned short INDEX[N_STRIPS][N_STRIP_INDICES][2]; // X, Y

void init_vertices_index() {
  // Initialize the vertices
  for (int x = 0; x < N_VERTICES_X; x++) {
    for (int y = 0; y < N_VERTICES_Y; y++) {
      VERTICES[x][y][vX] = x * TRIANGLE_WIDTH + DRAW_START_X;
      VERTICES[x][y][vY] = y * TRIANGLE_HEIGHT + DRAW_START_Y;
      VERTICES[x][y][vZ] = 1.0f;
      if (x == HUGE_INDEX_X && y == HUGE_INDEX_Y && HUGE_ST) {
        VERTICES[x][y][vS] = HUGE_VALUE_S;
        VERTICES[x][y][vT] = HUGE_VALUE_T;
      } else {
        VERTICES[x][y][vS] = (x * TRIANGLE_SIZE_U + DRAW_START_U) / (float)TEX_SIZE;
        VERTICES[x][y][vT] = (y * TRIANGLE_SIZE_V + DRAW_START_V) / (float)TEX_SIZE;
      }
      VERTICES[x][y][vQ] = 1.0f;
    }
  }

  // Initialize the index
  for (int y = 0; y < N_STRIPS; y++) {
    for (int x = 0; x < N_STRIP_INDICES; x++) {
      INDEX[y][x][0] = x / 2;
      INDEX[y][x][1] = y + (x % 2);
    }
  }
}

void init_texture_data() {
  for (int x = 0; x < TEX_SIZE; x++) {
    for (int y = 0; y < TEX_SIZE; y++) {
      if ((x < TEX_BORDER_SIZE) || (x >= TEX_SIZE - TEX_BORDER_SIZE) ||
        (y < TEX_BORDER_SIZE) || (y >= TEX_SIZE - TEX_BORDER_SIZE)) {
        TEXTURE_DATA[y][x][0] = 0x00;
        TEXTURE_DATA[y][x][1] = 0x00;
        TEXTURE_DATA[y][x][2] = 0x00;
        TEXTURE_DATA[y][x][3] = 0x00;
      } else if (((x / TEX_CHECKER_SIZE) & 1) ^ ((y / TEX_CHECKER_SIZE) & 1)) {
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
    }
  }
}

void init_gs(framebuffer_t* frame, zbuffer_t* z, texbuffer_t* texbuf)
{
  frame->width = FRAME_WIDTH;
  frame->height = FRAME_HEIGHT;
  frame->mask = 0;
  frame->psm = GS_PSM_32;
  frame->address = graph_vram_allocate(frame->width, frame->height, frame->psm, GRAPH_ALIGN_PAGE);

  // Disable the zbuffer.
  z->enable = DRAW_DISABLE;
  z->mask = 1;
  z->method = ZTEST_METHOD_ALLPASS;
  z->zsm = GS_ZBUF_32;
  z->address = graph_vram_allocate(frame->width, frame->height, z->zsm, GRAPH_ALIGN_PAGE);

  // Allocate some vram for the texture buffer
  texbuf->width = TEX_SIZE;
  texbuf->psm = GS_PSM_32;
  texbuf->address = graph_vram_allocate(TEX_SIZE, TEX_SIZE, GS_PSM_32, GRAPH_ALIGN_BLOCK);

  // Initialize the screen and tie the first framebuffer to the read circuits.
  int mode = graph_get_region();

	// Set a default interlaced video mode with flicker filter.
	graph_set_mode(GRAPH_MODE_NONINTERLACED, mode, GRAPH_MODE_FRAME, GRAPH_ENABLE);

	// Set the screen up
	graph_set_screen(0, 0, frame->width, frame->height);

	// Set black background
	graph_set_bgcolor(0, 0, 0);

	graph_set_framebuffer_filtered(frame->address, frame->width, frame->psm, 0, 0);

	graph_enable_output();
}

qword_t* my_draw_setup_environment(qword_t* q, int context, framebuffer_t* frame, zbuffer_t* z) {
	// Begin packed gif data packet with another qword.
	PACK_GIFTAG(q, GIF_SET_TAG(15, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	// Framebuffer setting
	PACK_GIFTAG(q, GS_SET_FRAME(frame->address >> 11, frame->width >> 6, frame->psm, frame->mask), GS_REG_FRAME);
	q++;
  PACK_GIFTAG(q, GS_SET_ZBUF(z->address >> 11, z->zsm, z->mask), GS_REG_ZBUF + context);
	q++;
  // Override Primitive Control
	PACK_GIFTAG(q, GS_SET_PRMODECONT(PRIM_OVERRIDE_DISABLE), GS_REG_PRMODECONT);
	q++;
	// Primitive coordinate offsets
	PACK_GIFTAG(q, GS_SET_XYOFFSET(OFX << 4, OFY << 4), GS_REG_XYOFFSET);
	q++;
	// Scissoring area
	PACK_GIFTAG(q, GS_SET_SCISSOR(0, frame->width - 1, 0, frame->height - 1), GS_REG_SCISSOR );
	q++;
  // Pixel testing
	PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS), GS_REG_TEST + context);
	q++;
	// Fog Color
	PACK_GIFTAG(q, GS_SET_FOGCOL(0, 0, 0), GS_REG_FOGCOL);
	q++;
	// Per-pixel Alpha Blending (Blends if MSB of ALPHA is true)
	PACK_GIFTAG(q, GS_SET_PABE(DRAW_DISABLE), GS_REG_PABE);
	q++;
  PACK_GIFTAG(q, GS_SET_ALPHA(BLEND_COLOR_SOURCE, BLEND_COLOR_DEST, BLEND_ALPHA_SOURCE,
								BLEND_COLOR_DEST, 0x80), GS_REG_ALPHA);
  q++;
	// Dithering
	PACK_GIFTAG(q, GS_SET_DTHE(GS_DISABLE), GS_REG_DTHE);
	q++;
  PACK_GIFTAG(q, GS_SET_DIMX(4, 2, 5, 3, 0, 6, 1, 7, 5, 3, 4, 2, 1, 7, 0, 6), GS_REG_DIMX);
	q++;
	// Color Clamp
	PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP);
	q++;
	// Texture wrapping/clamping
	PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_CLAMP, WRAP_CLAMP, 0, 0, 0, 0), GS_REG_CLAMP);
	q++;
  PACK_GIFTAG(q, GS_SET_FBA(ALPHA_CORRECT_RGBA32), GS_REG_FBA + context);
  q++;
  PACK_GIFTAG(q, GS_SET_TEXA(0x80, ALPHA_EXPAND_NORMAL, 0x80), GS_REG_TEXA);
	q++;
	return q;
}

void init_drawing_environment(framebuffer_t* frame, zbuffer_t* z) {
  packet_t* packet = packet_init(30, PACKET_NORMAL);

  // This is our generic qword pointer.
  qword_t* q = packet->data;

  // This will setup a default drawing environment.
  q = my_draw_setup_environment(q, 0, frame, z);

  // Finish setting up the environment.
  q = draw_finish(q);

  // Now send the packet, no need to wait since it's the first.
  dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
  dma_wait_fast();

  packet_free(packet);
}

void load_texture(texbuffer_t* texbuf) {
  packet_t* packet = packet_init(50, PACKET_NORMAL);

  qword_t* q = packet->data;

  q = draw_texture_transfer(q, TEXTURE_DATA, TEX_SIZE, TEX_SIZE, GS_PSM_32, texbuf->address, texbuf->width);
  q = draw_texture_flush(q);

  dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
  dma_wait_fast();

  packet_free(packet);
}

void setup_texture(texbuffer_t* texbuf) {
  packet_t* packet = packet_init(10, PACKET_NORMAL);

  qword_t* q = packet->data;

  // Using a texture involves setting up a lot of information.
  clutbuffer_t clut;

  lod_t lod;

  lod.calculation = LOD_USE_K;
  lod.max_level = 0;
  lod.mag_filter = LOD_MAG_NEAREST;
  lod.min_filter = LOD_MIN_NEAREST;
  lod.l = 0;
  lod.k = 0;

  texbuf->info.width = draw_log2(TEX_SIZE);
  texbuf->info.height = draw_log2(TEX_SIZE);
  texbuf->info.components = TEXTURE_COMPONENTS_RGBA;
  texbuf->info.function = TEXTURE_FUNCTION_DECAL;

  clut.storage_mode = CLUT_STORAGE_MODE1;
  clut.start = 0;
  clut.psm = 0;
  clut.load_method = CLUT_NO_LOAD;
  clut.address = 0;

  q = draw_texture_sampling(q, 0, &lod);
  q = draw_texturebuffer(q, 0, texbuf, &clut);

  // Now send the packet, no need to wait since it's the first.
  dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
  dma_wait_fast();

  packet_free(packet);
}

int render(framebuffer_t* frame, zbuffer_t* z) {
  packet_t* packet = packet_init(1024, PACKET_NORMAL);

  // Define the color we want to use for all vertices.
  color_t color;
  color.r = 0xFF;
  color.g = 0xFF;
  color.b = 0xFF;
  color.a = 0x80;
  color.q = 1.0f;

  // Define the color we want for background.
  color_t bg_color;
  bg_color.r = 0xFF;
  bg_color.g = 0;
  bg_color.b = 0;
  bg_color.a = 0x80;
  bg_color.q = 1.0f;

  // The main loop...
  while (true) {
    qword_t* q = packet->data;

    // Clear framebuffer
    PACK_GIFTAG(q, GIF_SET_TAG(7, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_TEST(1, ATEST_METHOD_GREATER_EQUAL, 0x80, ATEST_KEEP_ALL, 0, 0, 1, ZTEST_METHOD_ALLPASS), GS_REG_TEST);
    q++;
    PACK_GIFTAG(q, GIF_SET_PRIM(PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_REG_PRIM);
    q++;
    PACK_GIFTAG(q, bg_color.rgbaq, GIF_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GIF_SET_XYZ(OFX << 4, OFY << 4, 0), GIF_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, bg_color.rgbaq, GIF_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GIF_SET_XYZ((OFX + FRAME_WIDTH) << 4, (OFY + FRAME_HEIGHT) << 4, 0), GIF_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_TEST(1, ATEST_METHOD_GREATER_EQUAL, 0x80, ATEST_KEEP_ALL, 0, 0, 1, ZTEST_METHOD_ALLPASS), GS_REG_TEST);
    q++;

    // Set the prim register
    PACK_GIFTAG(q, GIF_SET_TAG(1, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_TRIANGLE_STRIP, PRIM_SHADE_GOURAUD, 1, 0, 1, 0, 0, 0, 0), GS_REG_PRIM);
    q++;

    // Draw the polys
    // Expect 3 vertices with 3 registers each in AD format
    PACK_GIFTAG(q, GIF_SET_TAG(N_STRIPS * N_STRIP_INDICES * 3, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // Add the vertices to the GIF packet
    for (int i = 0; i < N_STRIPS; i++) {
      for (int j = 0; j < N_STRIP_INDICES; j++) {
        // RGBAQ
        color.q = VERTICES[INDEX[i][j][0]][INDEX[i][j][1]][vQ];
        PACK_GIFTAG(q, color.rgbaq, GIF_REG_RGBAQ);
        q++;

        // ST
        union {
          struct {
            float s;
            float t;
          };
          u64 data;
        } data;
        data.s = (float)VERTICES[INDEX[i][j][0]][INDEX[i][j][1]][vS];
        data.t = (float)VERTICES[INDEX[i][j][0]][INDEX[i][j][1]][vT];
        PACK_GIFTAG(q, data.data, GIF_REG_ST);
        q++;

        // XYZ2
        unsigned short X_int = (unsigned short)((VERTICES[INDEX[i][j][0]][INDEX[i][j][1]][vX] + OFX) * 16.0f);
        unsigned short Y_int = (unsigned short)((VERTICES[INDEX[i][j][0]][INDEX[i][j][1]][vY] + OFY) * 16.0f);
        unsigned short Z_int = (unsigned short)VERTICES[INDEX[i][j][0]][INDEX[i][j][1]][vZ];
        if (j < 2) {
          PACK_GIFTAG(q, GIF_SET_XYZ(X_int, Y_int, Z_int), GIF_REG_XYZ3); // No drawing kick for first two vertices
        } else {
          PACK_GIFTAG(q, GIF_SET_XYZ(X_int, Y_int, Z_int), GIF_REG_XYZ2);
        }
        q++;
      }
    }

    // Setup a finish event.
    q = draw_finish(q);

    // Now send our current dma chain.
    dma_wait_fast();
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    // Wait for scene to finish drawing
    draw_wait_finish();
    // Wait for vsync
    graph_wait_vsync();
  }

  free(packet);

  // End program.
  return 0;
}

int main(int argc, char *argv[])
{
  // Make the TEXTURE_DATA texture.
  init_texture_data();

  // Make the vertices and index
  init_vertices_index();

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
