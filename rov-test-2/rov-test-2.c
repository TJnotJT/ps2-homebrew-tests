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
#include <math.h>

#ifndef MODE
#define MODE 0
#endif

#ifndef BLEND
#define BLEND 1
#endif

#ifndef ZMASK
#define ZMASK 1
#endif

#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))

#define PI 3.141592653589793f

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 224
#define WINDOW_X (2048 - FRAME_WIDTH / 2)
#define WINDOW_Y (2048 - FRAME_HEIGHT / 2)

#define TEX_BUF_WIDTH 256
#define TEX_WIDTH 256
#define TEX_HEIGHT 256

#define CTX_BASE 0
#define CTX_BLEND 1
#define N_CTX 2

#define TEX_BASE 0
#define TEX_BLEND 1

framebuffer_t g_frame; // Frame buffer
zbuffer_t g_z[N_CTX]; // Z buffer
texbuffer_t g_tex[N_CTX]; // Texture buffer

zbuffer_t g_z_clear; // Z buffer for clear

// Drawing context
clutbuffer_t g_clut;
lod_t g_lod;
texwrap_t g_wrap;
blend_t g_blend[N_CTX];
atest_t g_atest[N_CTX];
dtest_t g_dtest[N_CTX];
ztest_t g_ztest[N_CTX];

// Clearing context
blend_t g_blend_clear;
atest_t g_atest_clear;
dtest_t g_dtest_clear;
ztest_t g_ztest_clear;

u8 g_frame_data[FRAME_WIDTH * FRAME_HEIGHT * 4] __attribute__((aligned(64)));
u8 g_texture_data[N_CTX][TEX_WIDTH * TEX_HEIGHT * 4] __attribute__((aligned(64)));

typedef struct {
    float x, y, z;
    float s, t;
		u32 rgba;
} my_vertex_t;

// Cube vertices. Generated with ChatGPT.
const my_vertex_t g_cube_vertices[24] = {
    // ---- Front (+Z) ----
    { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0x800000ff }, // 0
    {  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0x800000ff }, // 1
    {  0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0x800000ff }, // 2
    { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0x800000ff }, // 3

    // ---- Back (-Z) ----
    {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0x8000ff00 }, // 4
    { -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0x8000ff00 }, // 5
    { -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0x8000ff00 }, // 6
    {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0x8000ff00 }, // 7

    // ---- Left (-X) ----
    { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0x80ff0000 }, // 8
    { -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0x80ff0000 }, // 9
    { -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0x80ff0000 }, // 10
    { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0x80ff0000 }, // 11

    // ---- Right (+X) ----
    {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0x80ff00ff }, // 12
    {  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0x80ff00ff }, // 13
    {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0x80ff00ff }, // 14
    {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0x80ff00ff }, // 15

    // ---- Top (+Y) ----
    { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 0x8000ffff }, // 16
    {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0x8000ffff }, // 17
    {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0x8000ffff }, // 18
    { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0x8000ffff }, // 19

    // ---- Bottom (-Y) ----
    { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0x80ffff00 }, // 20
    {  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0x80ffff00 }, // 21
    {  0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0x80ffff00 }, // 22
    { -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0x80ffff00 }, // 23
};

// Cube indices. Generated with ChatGPT.
const unsigned short g_cube_indices[36] = {
    0,1,2,  0,2,3,       // Front
    4,5,6,  4,6,7,       // Back
    8,9,10, 8,10,11,     // Left
    12,13,14, 12,14,15,  // Right
    16,17,18, 16,18,19,  // Top
    20,21,22, 20,22,23   // Bottom
};

// Vertices after applying perspective/viewpor transforms
my_vertex_t g_cube_vertices_transformed[24];

int max(int a, int b)
{
	return (a > b) ? a : b;
}

int min(int a, int b)
{
	return (a < b) ? a : b;
}

int abs(int x)
{
	return x < 0 ? -x : x;
}

// Perspective/viewport transform. Generated with ChatGPT.
void transform_vertices(float cx, float cy, float cz, float angle_x, float angle_y)
{
    const float fw = (float)FRAME_WIDTH;
    const float fh = (float)FRAME_HEIGHT;

    const float aspect = fw / fh;

    float cosx = cosf(angle_x * (PI / 180.0f));
    float sinx = sinf(angle_x * (PI / 180.0f));
    float cosy = cosf(angle_y * (PI / 180.0f));
    float siny = sinf(angle_y * (PI / 180.0f));

    for (int i = 0; i < COUNTOF(g_cube_vertices); i++)
    {
			float x = g_cube_vertices[i].x;
			float y = g_cube_vertices[i].y;
			float z = g_cube_vertices[i].z;

			// ---- Rotate around X ----
			float x1 = x;
			float y1 = y * cosx - z * sinx;
			float z1 = y * sinx + z * cosx;

			// ---- Rotate around Y ----
			float x2 = x1 * cosy + z1 * siny;
			float y2 = y1;
			float z2 = -x1 * siny + z1 * cosy;

			// ---- Translate by center ----
			float x3 = x2 + cx;
			float y3 = y2 + cy;
			float z3 = z2 + cz;

			// ---- Perspective divide (projection plane at z=1) ----
			float inv_z = 1.0f / z3;

			float xp = x3 * inv_z;
			float yp = y3 * inv_z;

			// ---- Aspect ratio correction ----
			xp /= aspect;

			// ---- Viewport transform ----
			float screen_x = (xp * 0.5f + 0.5f) * fw;
			float screen_y = (1.0f - (yp * 0.5f + 0.5f)) * fh;

			float s = g_cube_vertices[i].s;
			float t = g_cube_vertices[i].t;

			u32 rgba = g_cube_vertices[i].rgba;

			g_cube_vertices_transformed[i].x = screen_x;
			g_cube_vertices_transformed[i].y = screen_y;
			g_cube_vertices_transformed[i].z = inv_z;
			g_cube_vertices_transformed[i].s = s;
			g_cube_vertices_transformed[i].t = t;
			g_cube_vertices_transformed[i].rgba = rgba;
    }
}

// Texture for the base draw. In Black it's the diffuse texture,
// The alpha channel of this texture is used to modulate the specular texture
// on the second draw with the blend set to Cs * Ad + Cd.
void make_base_texture()
{
	for (int y = 0; y < TEX_HEIGHT; y++)
	{
		for (int x = 0; x < TEX_WIDTH; x++)
		{
			const int i = (y * TEX_WIDTH + x) * 4; // Pixel index
			
			// Gradient for base texture
			const int dx = abs(2 * x - TEX_WIDTH);
			const int dy = abs(2 * y - TEX_HEIGHT);
			const float df = ((float)dx + (float)dy) / (float)(TEX_WIDTH + TEX_HEIGHT);
			int gray = (int)(0x20 + (1.0f - df) * 0x40);

			g_texture_data[TEX_BASE][i + 0] = gray;
			g_texture_data[TEX_BASE][i + 1] = gray;
			g_texture_data[TEX_BASE][i + 2] = gray;
			g_texture_data[TEX_BASE][i + 3] = gray;
		}
	}

	FlushCache(0); // Needed for DMA transfer since we generate data on the CPU.
}

// Texture used for the blend draws. In Black it's the specular texture.
void make_blend_texture()
{
	for (int y = 0; y < TEX_HEIGHT; y++)
	{
		for (int x = 0; x < TEX_WIDTH; x++)
		{
			const int i = (y * TEX_WIDTH + x) * 4; // Pixel index

			// Checkerboard for the blending texture
			if (((x / 16) + (y / 16)) & 1)
			{
				g_texture_data[TEX_BLEND][i + 0] = 0x20;
				g_texture_data[TEX_BLEND][i + 1] = 0x20;
				g_texture_data[TEX_BLEND][i + 2] = 0x20;
				g_texture_data[TEX_BLEND][i + 3] = 0xFF;
			}
			else
			{
				g_texture_data[TEX_BLEND][i + 0] = 0x40;
				g_texture_data[TEX_BLEND][i + 1] = 0x40;
				g_texture_data[TEX_BLEND][i + 2] = 0x40;
				g_texture_data[TEX_BLEND][i + 3] = 0xFF;
			}
		}
	}

	FlushCache(0); // Needed for DMA transfer since we generate data on the CPU.
}

// Send texture to GS VRAM
void load_texture(u8* texture_data, texbuffer_t* texbuf)
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);

	qword_t* q = packet;

	q = draw_texture_transfer(q, texture_data, 1 << texbuf->info.width, 1 << texbuf->info.height,
		texbuf->psm, texbuf->address, texbuf->width);
	q = draw_texture_flush(q);

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
	// Frame
	g_frame.width = FRAME_WIDTH;
	g_frame.height = FRAME_HEIGHT;
	g_frame.mask = 0;
	g_frame.psm = GS_PSM_32;
	g_frame.address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, g_frame.psm, GRAPH_ALIGN_PAGE);

	// Z buffer
	g_z[CTX_BASE].enable = DRAW_ENABLE;
	g_z[CTX_BASE].mask = 0;
	g_z[CTX_BASE].method = ZTEST_METHOD_ALLPASS;
	g_z[CTX_BASE].zsm = GS_ZBUF_24;
	g_z[CTX_BASE].address = graph_vram_allocate(FRAME_WIDTH, FRAME_HEIGHT, GS_PSMZ_24, GRAPH_ALIGN_PAGE);

	// Same as base
	g_z[CTX_BLEND] = g_z[CTX_BASE];

	// Same as base
	g_z_clear = g_z[CTX_BASE];

	// Texture buffers
	for (int i = 0; i < N_CTX; i++)
	{
		g_tex[i].width = TEX_BUF_WIDTH;
		g_tex[i].psm = GS_PSM_32;
		g_tex[i].address = graph_vram_allocate(TEX_BUF_WIDTH, TEX_HEIGHT, GS_PSM_32, GRAPH_ALIGN_BLOCK);

		g_tex[i].info.width = draw_log2(TEX_WIDTH);
		g_tex[i].info.height = draw_log2(TEX_HEIGHT);
		g_tex[i].info.function = TEXTURE_FUNCTION_MODULATE;
		g_tex[i].info.components = TEXTURE_COMPONENTS_RGBA;
	}

	// CLUT (not used)
	g_clut.storage_mode = CLUT_STORAGE_MODE1;
	g_clut.start = 0;
	g_clut.psm = 0;
	g_clut.load_method = CLUT_NO_LOAD;
	g_clut.address = 0;

	// Set MAG linear and K = 0 so that linear is used.
	g_lod.calculation = LOD_USE_K;
	g_lod.max_level = 0;
	g_lod.mag_filter = LOD_MAG_LINEAR;
	g_lod.min_filter = LOD_MIN_NEAREST;
	g_lod.l = 0;
	g_lod.k = 0.0f;

	g_wrap.horizontal = WRAP_CLAMP;
	g_wrap.vertical = WRAP_CLAMP;
	g_wrap.minu = 0;
	g_wrap.maxu = 0 ;
	g_wrap.minv = 0;
	g_wrap.maxv = 0;

	// Main context settings
	g_blend[CTX_BASE].color1 = BLEND_COLOR_SOURCE;
	g_blend[CTX_BASE].color2 = BLEND_COLOR_ZERO;
	g_blend[CTX_BASE].color3 = BLEND_COLOR_ZERO;
	g_blend[CTX_BASE].alpha = BLEND_ALPHA_FIXED;
	g_blend[CTX_BASE].fixed_alpha = 0x80;

	g_atest[CTX_BASE].compval = 0;
	g_atest[CTX_BASE].enable = DRAW_ENABLE;
	g_atest[CTX_BASE].keep = ATEST_KEEP_ALL;
	g_atest[CTX_BASE].method = ATEST_METHOD_ALLPASS;

	g_dtest[CTX_BASE].enable = DRAW_DISABLE;
	g_dtest[CTX_BASE].pass = DTEST_METHOD_PASS_ZERO;

	g_ztest[CTX_BASE].enable = DRAW_ENABLE;
	g_ztest[CTX_BASE].method = ZTEST_METHOD_GREATER_EQUAL;

	// Blend context settings

	// Blend copied from Black game: Cs * Ad + Cd
	g_blend[CTX_BLEND].color1 = BLEND_COLOR_SOURCE;
	g_blend[CTX_BLEND].color2 = BLEND_COLOR_ZERO;
	g_blend[CTX_BLEND].color3 = BLEND_COLOR_DEST;
	g_blend[CTX_BLEND].alpha = BLEND_ALPHA_DEST;
	g_blend[CTX_BLEND].fixed_alpha = 0x80;

	// Mask Z by using ALLFAIL alpha test and setting AFAIL to KEEP_ZBUFFER
	g_atest[CTX_BLEND].compval = 0;
	g_atest[CTX_BLEND].enable = ZMASK ? DRAW_ENABLE : DRAW_DISABLE;
	g_atest[CTX_BLEND].keep = ATEST_KEEP_ZBUFFER;
	g_atest[CTX_BLEND].method = ATEST_METHOD_ALLFAIL;

	// Same as base
	g_dtest[CTX_BLEND] = g_dtest[CTX_BASE];
	g_ztest[CTX_BLEND] = g_ztest[CTX_BASE];

	// Clear context settings (disable most things for a clear)
	g_blend_clear.color1 = BLEND_COLOR_ZERO;
	g_blend_clear.color2 = BLEND_COLOR_ZERO;
	g_blend_clear.color3 = BLEND_COLOR_SOURCE;
	g_blend_clear.alpha = BLEND_ALPHA_FIXED;
	g_blend_clear.fixed_alpha = 0x80;

	g_atest_clear.compval = 0;
	g_atest_clear.enable = DRAW_DISABLE;
	g_atest_clear.keep = ATEST_KEEP_ALL;
	g_atest_clear.method = ATEST_METHOD_ALLPASS;

	g_dtest_clear.enable = DRAW_DISABLE;
	g_dtest_clear.pass = DTEST_METHOD_PASS_ONE;

	g_ztest_clear.enable = DRAW_ENABLE;
	g_ztest_clear.method = ZTEST_METHOD_ALLPASS;

	// Initialize privileged registers
	graph_initialize_custom();
}

// Clear screen with a quad
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

// Draws the cube with alternating context to mimic Black drawing
qword_t* my_draw_cube(qword_t* q)
{
	const int tris = COUNTOF(g_cube_indices) / 3;

	PACK_GIFTAG(q, GIF_SET_TAG(N_CTX * (1 + 9 * tris), 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

	for (int j = 0; j < N_CTX; j++)
	{
		// Switch contexts every cube
		PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_TRIANGLE, 0, 1, 0, !!BLEND, 0, 0, j, 0), GIF_REG_PRIM);
		q++;

		for (int i = 0; i < tris; i++)
		{
			for (int k = 0; k < 3; k++)
			{
				const my_vertex_t* v = &g_cube_vertices_transformed[g_cube_indices[3 * i + k]];

				const int x = (WINDOW_X << 4) + (int)(v->x * 16.0f);
				const int y = (WINDOW_Y << 4) + (int)(v->y * 16.0f);
				const int z = (int)(v->z * (float)((1 << 24) - 1));

				union
				{
					struct
					{
						float s, t;
					};
					u64 U64;
				} st;

				st.s = v->s;
				st.t = v->t;

				color_t color;
				color.rgbaq = v->rgba;
				color.q = 1.0f;

				PACK_GIFTAG(q, color.rgbaq, GS_REG_RGBAQ);
				q++;

				PACK_GIFTAG(q, st.U64, GS_REG_ST);
				q++;

				PACK_GIFTAG(q, GS_SET_XYZ(x, y, z), GS_REG_XYZ2);
				q++;
			}
		}
	}

	return q;
}

// Work around a PS2 bug by sending a dummy EOP packet
void send_dummy_eop()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 8);
	qword_t* q;

	q = packet;
	q = draw_finish(q);
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	draw_wait_finish();

	free(packet);
}

// Set up the drawing environment
qword_t* setup_drawing_env(qword_t* q)
{
	for (int i = 0; i < N_CTX; i++)
	{
		q = draw_setup_environment(q, i, &g_frame, &g_z[i]);
		q = draw_texture_sampling(q, i, &g_lod);
		q = draw_texturebuffer(q, i, &g_tex[i], &g_clut);
		q = draw_texture_wrapping(q, i, &g_wrap);
		q = draw_alpha_blending(q, i, &g_blend[i]);
		q = draw_primitive_xyoffset(q, i, (float)WINDOW_X, (float)WINDOW_Y);
		q = draw_pixel_test(q, i, &g_atest[i], &g_dtest[i], &g_ztest[i]);
	}

	return q;
}

// Setup the clear environment
qword_t* setup_clear_env(qword_t* q)
{
	q = draw_setup_environment(q, 0, &g_frame, &g_z_clear);
	q = draw_alpha_blending(q, 0, &g_blend_clear);
	q = draw_primitive_xyoffset(q, 0, (float)WINDOW_X, (float)WINDOW_Y);
	q = draw_pixel_test(q, 0, &g_atest_clear, &g_dtest_clear, &g_ztest_clear);

	return q;
}

// Main rendering function
int render_test()
{
	srand(123);

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 65536);
	qword_t* q;

	q = packet;
	q = setup_clear_env(q);
	q = my_draw_clear(q, 0, 0);

	q = setup_drawing_env(q);

#if MODE == 0
	// Light: just one cube.
	transform_vertices(0.0f, 0.0f, 2.0f, 45.0f, 45.0f);

	q = my_draw_cube(q);
#else
	// Heavy: a lot of cubes.
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			for (int k = 0; k < 7; k++)
			{
				transform_vertices(1.25f * (float)(j - 5), 1.25f * (float)(k - 3), 1.25f * (float)(7 - i), 0.0f, 0.0f);

				q = my_draw_cube(q);
			}
		}

		if (q - packet >= 65536 / 2)
		{
			dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
			dma_channel_wait(DMA_CHANNEL_GIF, 0);
			q = packet;
		}
	}
#endif

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

	send_dummy_eop();

	make_base_texture();

	make_blend_texture();

	for (int i = 0; i < N_CTX; i++)
	{
		load_texture(g_texture_data[i], &g_tex[i]);
	}

	while (1)
	{
		render_test();
	}

	SleepThread();

	return 0;
}
