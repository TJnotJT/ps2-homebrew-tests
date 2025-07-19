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

#define SWAP(x, y) \
  do { \
    typeof(x) temp = (x); \
    (x) = (y); \
    (y) = temp; \
  } while (0)

#define SETTING_READ_RG_WRITE_BA           0x0
#define SETTING_READ_BA_WRITE_RG           0x1
#define SETTING_READ_RGBA_WRITE_RGBA       0x2
#define SETTING_READ_WRITE_MASK            0x3
#define SETTING_ACROSS_X                   0x4
#define SETTING_ACROSS_U                   0x8
#define SETTING_ACROSS_MASK                0xC
#define SETTING_REV_X                     0x10
#define SETTING_REV_Y                     0x20
#define SETTING_REV_U                     0x40
#define SETTING_REV_V                     0x80
#define SETTING_REV_MASK                  0xF0
#define SETTING_SPLIT                    0x100
#define SETTING_SPLIT_CHANGE_DST_ADDR    0x200
#define SETTING_SPLIT_CHANGE_SRC_ADDR    0x400
#define SETTING_SPLIT_MASK               0x700    

#define SETTING                SETTING_READ_RGBA_WRITE_RGBA | SETTING_SPLIT_CHANGE_DST_ADDR | SETTING_SPLIT_CHANGE_SRC_ADDR

#define SETTING_GET_READ_WRITE            ((SETTING) & SETTING_READ_WRITE_MASK)
#define SETTING_GET_ACROSS                ((SETTING) & SETTING_ACROSS_MASK)
#define SETTING_GET_REV_X                 ((SETTING) & SETTING_REV_X)
#define SETTING_GET_REV_Y                 ((SETTING) & SETTING_REV_Y)
#define SETTING_GET_REV_U                 ((SETTING) & SETTING_REV_U)
#define SETTING_GET_REV_V                 ((SETTING) & SETTING_REV_V)
#define SETTING_GET_SPLIT                 ((SETTING) & SETTING_SPLIT_MASK)
#define SETTING_GET_SPLIT_CHANGE_DST_ADDR ((SETTING) & SETTING_SPLIT_CHANGE_DST_ADDR)
#define SETTING_GET_SPLIT_CHANGE_SRC_ADDR ((SETTING) & SETTING_SPLIT_CHANGE_SRC_ADDR)

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 448
#define TEXTURE_WIDTH 128
#define TEXTURE_HEIGHT 64
#define TEX_COLOR_SRC_1 0x000000FF
#define TEX_COLOR_SRC_2 0x000000FF
#define TEX_COLOR_DST_1 0x00FF0000
#define TEX_COLOR_DST_2 0x00FF0000
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)
#define DRAW_X0 (2048 - TEXTURE_WIDTH / 2)
#define DRAW_Y0 (2048 - TEXTURE_HEIGHT / 2)
#define DRAW_X1 (2048 + TEXTURE_WIDTH / 2)
#define DRAW_Y1 (2048 + TEXTURE_HEIGHT / 2)

#define TEXTURE_SRC 0
#define TEXTURE_DST 1

framebuffer_t g_frame[2]; // Frame buffer
u32 g_frame_i = 0;              // Current back buffer 
zbuffer_t g_z;            // Z buffer
texbuffer_t g_texbuf_c32[2];  // Texture buffers as 32 bit color
texbuffer_t g_texbuf_c16[2];  // Texture buffers as 16 bit color (aliases of the 32-bit buffers)
framebuffer_t g_frame_shuffle; // Shuffle buffer (alias of one of the texture buffers)
clutbuffer_t g_clutbuf;
lod_t g_lod;
texwrap_t g_wrap;

u32 texture_data_src[TEXTURE_WIDTH * TEXTURE_HEIGHT];
u32 texture_data_dst[TEXTURE_WIDTH * TEXTURE_HEIGHT];

#define texture_data_src_uncached ((u32*)UNCACHED_SEG(texture_data_src))
#define texture_data_dst_uncached ((u32*)UNCACHED_SEG(texture_data_dst))

void init_texture()
{
  for (int y = 0; y < TEXTURE_HEIGHT; y++)
  {
    for (int x = 0; x < TEXTURE_WIDTH; x++)
    {
      if ((y & 1) == 0)
      {
        texture_data_src_uncached[y * TEXTURE_WIDTH + x] = TEX_COLOR_SRC_1;
        texture_data_dst_uncached[y * TEXTURE_WIDTH + x] = TEX_COLOR_DST_1;
      }
      else
      {
        texture_data_src_uncached[y * TEXTURE_WIDTH + x] = TEX_COLOR_SRC_2;
        texture_data_dst_uncached[y * TEXTURE_WIDTH + x] = TEX_COLOR_DST_2;
      }
    }
  }
  FlushCache(0); // Flush the texture data to main memory
}

void load_texture()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);
	qword_t* q = packet;

	q = draw_texture_transfer(q, texture_data_src, TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_PSM_32,
    g_texbuf_c32[TEXTURE_SRC].address, g_texbuf_c32[TEXTURE_SRC].width);
  q = draw_texture_transfer(q, texture_data_dst, TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_PSM_32,
    g_texbuf_c32[TEXTURE_DST].address, g_texbuf_c32[TEXTURE_DST].width);
	q = draw_texture_flush(q);

	FlushCache(0); // Needed because we generate data in the EE.

	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	dma_channel_send_chain(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);

	free(packet);
}

int graph_initialize_custom()
{
	int mode = graph_get_region();
	graph_set_mode(GRAPH_MODE_INTERLACED, mode, GRAPH_MODE_FIELD, GRAPH_ENABLE);
	graph_set_screen(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
	graph_set_bgcolor(0, 0, 0);
	graph_set_framebuffer(0, g_frame[g_frame_i].address, g_frame[g_frame_i].width, g_frame[g_frame_i].psm, 0, 0);
	graph_set_output(1, 0, 1, 0, 1, 0xFF);
	return 0;
}

void init_gs()
{
  for (int i = 0; i < 2; i++)
  {
    g_frame[i].width = FRAME_WIDTH;
    g_frame[i].height = FRAME_HEIGHT;
    g_frame[i].mask = 0;
    g_frame[i].psm = GS_PSM_32;
    g_frame[i].address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSM_32, GRAPH_ALIGN_PAGE);
  }

	g_z.enable = DRAW_DISABLE;
	g_z.mask = 0;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = 0;
	g_z.address = 0;

  for (int i = 0; i < 2; i++)
  {
    g_texbuf_c32[i].address = graph_vram_allocate(TEXTURE_WIDTH, TEXTURE_HEIGHT, GS_PSM_32, GRAPH_ALIGN_PAGE);
    g_texbuf_c32[i].width = TEXTURE_WIDTH;
    g_texbuf_c32[i].psm = GS_PSM_32;
    g_texbuf_c32[i].info.width = draw_log2(TEXTURE_WIDTH);
    g_texbuf_c32[i].info.height = draw_log2(TEXTURE_HEIGHT);
    g_texbuf_c32[i].info.components = TEXTURE_COMPONENTS_RGBA;
    g_texbuf_c32[i].info.function = TEXTURE_FUNCTION_DECAL;

    g_texbuf_c16[i].address = g_texbuf_c32[i].address; // Use the same address for 16-bit texture
    g_texbuf_c16[i].width = TEXTURE_WIDTH; // Keep the same width
    g_texbuf_c16[i].psm = GS_PSM_16;
    g_texbuf_c16[i].info.width = draw_log2(TEXTURE_WIDTH);
    g_texbuf_c16[i].info.height = draw_log2(TEXTURE_HEIGHT) + 1; // Double the height
    g_texbuf_c16[i].info.components = TEXTURE_COMPONENTS_RGBA;
    g_texbuf_c16[i].info.function = TEXTURE_FUNCTION_DECAL;
  }

  g_frame_shuffle.address = g_texbuf_c32[TEXTURE_DST].address;
  g_frame_shuffle.width = TEXTURE_WIDTH;
  g_frame_shuffle.height = TEXTURE_HEIGHT * 2; // Double the height for shuffle
  g_frame_shuffle.psm = GS_PSM_16;
  g_frame_shuffle.mask = 0;

  g_lod.calculation = LOD_USE_K;
  g_lod.max_level = 0;
  g_lod.mag_filter = LOD_MAG_NEAREST;
  g_lod.min_filter = LOD_MIN_NEAREST;
  g_lod.l = 0;
  g_lod.k = 0;

  g_clutbuf.storage_mode = CLUT_STORAGE_MODE1;
  g_clutbuf.start = 0;
  g_clutbuf.psm = 0;
  g_clutbuf.load_method = CLUT_NO_LOAD;
  g_clutbuf.address = 0;

  g_wrap.horizontal = WRAP_REPEAT;
  g_wrap.vertical = WRAP_REPEAT;
  g_wrap.minu = 0;
  g_wrap.maxu = TEXTURE_WIDTH - 1;
  g_wrap.minv = 0;
  g_wrap.maxv = TEXTURE_HEIGHT - 1;

  // Send a dummy EOP packet
  qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8192);
	qword_t* q = packet;
  PACK_GIFTAG(q, GIF_SET_TAG(1, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
  PACK_GIFTAG(q, 1, GS_REG_TEXFLUSH);
  q++;
	q = draw_finish(q);

  FlushCache(0); // Flush the cache to ensure all data is written

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();
}

qword_t* my_draw_disable_tests(qword_t* q, int context)
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
	PACK_GIFTAG(q, GIF_SET_XYZ(0xFFFF, 0, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(0, 0xFFFF, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(0xFFFF, 0xFFFF, 0), GIF_REG_XYZ2);
	q++;
	return q;
}

int render_sprites()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 2048);
	qword_t* q = packet;
  
  q = draw_setup_environment(q, 0, &g_frame[g_frame_i], &g_z);
  q = draw_primitive_xyoffset(q, 0, WINDOW_X, WINDOW_Y);
  q = draw_texture_sampling(q, 0, &g_lod);
  q = draw_texture_wrapping(q, 0, &g_wrap);
  q = my_draw_disable_tests(q, 0);
  q = draw_finish(q);

  q = my_draw_clear(q, 0x80FFFFFF);

  for (int i = 0; i < 2; i++)
  { 
    q = draw_texturebuffer(q, 0, &g_texbuf_c32[i], &g_clutbuf);

    u32 x0 = WINDOW_X + FRAME_WIDTH / 2 - TEXTURE_WIDTH / 2;
    u32 y0 = WINDOW_Y + FRAME_HEIGHT / 2 - TEXTURE_HEIGHT * (1 - i);
    u32 x1 = x0 + TEXTURE_WIDTH;
    u32 y1 = y0 + TEXTURE_HEIGHT;

    u32 u0 = 0;
    u32 v0 = 0;
    u32 u1 = u0 + TEXTURE_WIDTH;
    u32 v1 = v0 + TEXTURE_HEIGHT;

    PACK_GIFTAG(q, GIF_SET_TAG(7, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_PRIM(PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0), GIF_REG_PRIM);
    q++;
    PACK_GIFTAG(q, GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000), GIF_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GIF_SET_UV(u0 << 4, v0 << 4), GIF_REG_UV);
    q++;
    PACK_GIFTAG(q, GIF_SET_XYZ(x0 << 4, y0 << 4, 0), GIF_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000), GIF_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GIF_SET_UV(u1 << 4, v1 << 4), GIF_REG_UV);
    q++;
    PACK_GIFTAG(q, GIF_SET_XYZ(x1 << 4, y1 << 4, 0), GIF_REG_XYZ2);
    q++;
  }

  q = draw_finish(q);

  dma_channel_wait(DMA_CHANNEL_GIF, 0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
  draw_wait_finish();
  
  graph_wait_vsync();

  // Update the framebuffer
  graph_set_framebuffer(0, g_frame[g_frame_i].address, g_frame[g_frame_i].width, g_frame[g_frame_i].psm, 0, 0);
  
  g_frame_i ^= 1; // Switch to the next frame buffer

	free(packet);
	return 0;
}

void do_shuffle()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 2048);
	qword_t* q;

  // Set up the drawing environment
	q = packet;
	q = draw_setup_environment(q, 0, &g_frame_shuffle, &g_z);
  q = draw_texturebuffer(q, 0, &g_texbuf_c16[TEXTURE_SRC], &g_clutbuf);
	q = draw_primitive_xyoffset(q, 0, 0, 0);
  q = draw_texture_sampling(q, 0, &g_lod);
  q = draw_texture_wrapping(q, 0, &g_wrap);
  q = draw_texture_expand_alpha(q, 0x00, ALPHA_EXPAND_NORMAL, 0x80);
  q = draw_alpha_correction(q, 0, 0);
	q = my_draw_disable_tests(q, 0);
	q = draw_finish(q);

  qword_t* giftag = q; // Save for filling in later
  q++;

  qword_t* regs_start = q; // Start of the registers

  // Sprite, textured, UV coordinates
  u64 sprite_prim = GS_SET_PRIM(PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0);

  int num_draws_x;
  int num_draws_y;
  int pixels_x;
  int pixels_y;
  if (SETTING_GET_SPLIT)
  {
    num_draws_x = 2;
    num_draws_y = 2;
    pixels_x = TEXTURE_WIDTH / 2;
    pixels_y = TEXTURE_HEIGHT / 2;
  }
  else
  {
    num_draws_x = 1;
    num_draws_y = 1;
    pixels_x = TEXTURE_WIDTH;
    pixels_y = TEXTURE_HEIGHT;
  }
  
  int pixels_per_draw = pixels_x * pixels_y;

  for (int draw_y = 0; draw_y < num_draws_y; draw_y++)
  {
    for (int draw_x = 0; draw_x < num_draws_x; draw_x++)
    {
      int draw = draw_y * num_draws_x + draw_x;

      int x_base = 0;
      int y_base = 0;
      int u_base = 0;
      int v_base = 0;
      texbuffer_t src = g_texbuf_c16[TEXTURE_SRC];
      framebuffer_t dst = g_frame_shuffle;

      if (SETTING_GET_SPLIT)
      {
        if (SETTING_GET_SPLIT_CHANGE_SRC_ADDR)
        {
          src.address += draw * pixels_per_draw;
        }
        else
        {
          u_base = draw_x * pixels_x;
          v_base = 2 * draw_y * pixels_y; // double for C16
        }
        if (SETTING_GET_SPLIT_CHANGE_DST_ADDR)
        {
          dst.address += draw * pixels_per_draw;
        }
        else
        {
          x_base = draw_x * pixels_x;
          y_base = 2 * draw_y * pixels_y; // double for C16
        }
        
      }
      if (src.address != g_texbuf_c16[TEXTURE_SRC].address)
      {
        q = draw_texturebuffer(q, 0, &src, &g_clutbuf);
      }
      if (dst.address != g_frame_shuffle.address)
      {
        q = draw_framebuffer(q, 0, &dst);
      }
      PACK_GIFTAG(q, sprite_prim, GIF_REG_PRIM);
      q++;
      for (int i = 0; i < pixels_x; i += 16)
      {
        int x0, y0, x1, y1;
        int u0, v0, u1, v1;
        int width;

        y0 = 0;
        y1 = 2 * pixels_y;
        v0 = 0;
        v1 = 2 * pixels_y;

        // Set x0 and u0
        switch (SETTING_GET_READ_WRITE)
        {
          default:
          case SETTING_READ_RG_WRITE_BA:
            x0 = i + 8;
            u0 = i;
            width = 8;
            break;
          case SETTING_READ_BA_WRITE_RG:
            x0 = i;
            u0 = i + 8;
            width = 8;
            break;
          case SETTING_READ_RGBA_WRITE_RGBA:
            x0 = i;
            u0 = i;
            width = 16;
            break;
        }

        // Adjust x0 and u0 based on across setting
        if (SETTING_GET_ACROSS == SETTING_ACROSS_X)
        {
          if (SETTING_GET_READ_WRITE == SETTING_READ_RG_WRITE_BA)
          {
            if (x0 + 16 <= pixels_x)
              width = 16;
          }
          else if (SETTING_GET_READ_WRITE == SETTING_READ_BA_WRITE_RG)
          {
            if (x0 - 8 >= 0)
            {
              x0 -= 8;
              u0 -= 8;
              width = 16;
            }
          }
        }
        else if (SETTING_GET_ACROSS == SETTING_ACROSS_U)
        {
          if (SETTING_GET_READ_WRITE == SETTING_READ_RG_WRITE_BA)
          {
            if (u0 - 8 >= 0)
            {
              u0 -= 8;
              x0 -= 8;
              width = 16;
            }
          }
          else if (SETTING_GET_READ_WRITE == SETTING_READ_BA_WRITE_RG)
          {
            if (u0 + 16 <= pixels_x)
              width = 16;
          }
        }

        x1 = x0 + width;
        u1 = u0 + width;

        if (SETTING_GET_REV_X)
        {
          SWAP(x0, x1);
        }
        if (SETTING_GET_REV_U)
        {
          SWAP(u0, u1);
        }
        if (SETTING_GET_REV_Y)
        {
          SWAP(y0, y1);
        }
        if (SETTING_GET_REV_V)
        {
          SWAP(v0, v1);
        }

        x0 += x_base;
        x1 += x_base;
        y0 += y_base;
        y1 += y_base;
        u0 += u_base;
        u1 += u_base;
        v0 += v_base;
        v1 += v_base;

        PACK_GIFTAG(q, GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000), GIF_REG_RGBAQ);
        q++;
        PACK_GIFTAG(q, GIF_SET_UV((u0 << 4 ) | 0x8, (v0 << 4) | 0x8), GIF_REG_UV);
        q++;
        PACK_GIFTAG(q, GIF_SET_XYZ(x0 << 4, y0 << 4, 0), GIF_REG_XYZ2);
        q++;
        PACK_GIFTAG(q, GIF_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000), GIF_REG_RGBAQ);
        q++;
        PACK_GIFTAG(q, GIF_SET_UV((u1 << 4) | 0x8, (v1 << 4) | 0x8), GIF_REG_UV);
        q++;
        PACK_GIFTAG(q, GIF_SET_XYZ(x1 << 4, y1 << 4, 0), GIF_REG_XYZ2);
        q++;
      }

      int num_regs = q - regs_start; // Number of registers filled
      PACK_GIFTAG(giftag, GIF_SET_TAG(num_regs, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q = draw_finish(q);
    }
  }

  dma_channel_wait(DMA_CHANNEL_GIF, 0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
  draw_wait_finish();
}

int main(int argc, char *argv[])
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);

  graph_initialize_custom();

  init_gs();

  init_texture();

  load_texture();

  while (1)
  {
    render_sprites();
    do_shuffle();
  }

	SleepThread();

	return 0;
}
