#include <kernel.h>
#include <stdlib.h>
#include <malloc.h>
#include <tamtypes.h>
#include <stdio.h>
#include <string.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <gs_privileged.h>
#include <gs_gp.h>
#include <graph.h>
#include <dma.h>
#include <draw.h>

#include "main.h"
#include "swizzle.h"

#include "../lib-usb/usb.h"
#include "../lib-bmp/bmp.h"
#include "../lib-read-fb/read-fb.h"

#define READ_USB 1
#define READ_MEM 2
#ifndef READ_MODE
#error "READ_MODE must be defined as READ_USB or READ_MEM"
#endif

#define SAVE_FRAME 3                 // Which frame to save to file
#define SAVE_FRAME_NAME "frame3_counter.bmp"     // Name of the frame to save
#define SAVE_FRAME_ADDR (0x0 * 64) // Frame buffer pointer
#define SAVE_FRAME_WIDTH 512         // Frame buffer width
#define SAVE_FRAME_HEIGHT 512        // Frame buffer height
#define SAVE_FRAME_PSM GS_PSM_16S

#define DEBUG_FRAME_WIDTH 512
#define DEBUG_FRAME_HEIGHT 256
#define DEBUG_WINDOW_X 0
#define DEBUG_WINDOW_Y 0

#define TRANSFER_SIZE 0x10000
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
// #define PRINTF(fmt, ...)

framebuffer_t g_frame; // Frame buffer for debugging
zbuffer_t g_z;         // Z buffer
u8 frame_data[4 * SAVE_FRAME_WIDTH * SAVE_FRAME_HEIGHT] __attribute__((aligned(64))); // For reading frame buffer back

typedef struct {
  u64 RC : 3;
  u64 LC : 7;
  u64 T1248 : 2;
  u64 SLCK : 1;
  u64 CMOD : 2;
  u64 EX : 1;
  u64 PRST : 1;
  u64 SINT : 1;
  u64 XPCK : 1;
  u64 PCK2 : 2;
  u64 SPML : 4;
  u64 GCONT : 1;
  u64 PHS : 1;
  u64 PVS : 1;
  u64 PEHS : 1;
  u64 PEVS : 1;
  u64 CLKSEL : 2;
  u64 NVCK : 1;
  u64 SLCK2 : 1;
  u64 VCKSEL : 2;
  u64 VHP : 2;
} SMODE1_t;

// gs_dump_t dump __attribute__((aligned(16))); // For storing the GS dump

gs_dump_header_t gs_dump_header;
qword_t transfer_buffer[TRANSFER_SIZE] __attribute__((aligned(16))); // For transferring GS packets with qword alignment
gs_registers_t register_buffer __attribute__((aligned(16))); // For transferring GS privileged registers with qword alignment
u8 gs_dump_state[0x401000] __attribute__((aligned(16)));

extern GRAPH_MODE graph_mode[22]; // For inferring the video mode
extern u64 smode1_values[22]; // For inferring the video mode

#if READ_MODE == READ_USB
FILE* pcsx2_dump_file = NULL;
#elif READ_MODE == READ_MEM
extern u32 size_pcsx2_dump;
extern u8 pcsx2_dump[] __attribute__((aligned(16)));
u8* pcsx2_dump_ptr = NULL;
#endif

void fail()
{
	PRINTF("Failed\n");
	while (1)
		SleepThread();
}

int my_read_init()
{
#if READ_MODE == READ_USB
	if (pcsx2_dump_file)
	{
		if (fseek(pcsx2_dump_file, 0, SEEK_SET) != 0)
		{
			PRINTF("Failed to seek to start of pcsx2_dump.gs\n");
			fail();
			return -1;
		}
		PRINTF("Rewound pcsx2_dump.gs\n");
		return 0;
	}
	else
	{
		pcsx2_dump_file = fopen("mass:pcsx2_dump.gs", "r");
		if (!pcsx2_dump_file)
		{
			PRINTF("Failed to open pcsx2_dump.gs\n");
			fail();
			return -1;
		}
		PRINTF("Opened pcsx2_dump.gs\n");
	}
#elif READ_MODE == READ_MEM
	pcsx2_dump_ptr = pcsx2_dump;
#endif
	return 0;
}

int my_read_eof()
{
#if READ_MODE == READ_USB
	return feof(pcsx2_dump_file);
#elif READ_MODE == READ_MEM
	return pcsx2_dump_ptr - pcsx2_dump >= size_pcsx2_dump;
#endif
}

int my_read_close()
{
#if READ_MODE == READ_USB
	if (pcsx2_dump_file)
	{
		if (fclose(pcsx2_dump_file) != 0)
		{
			PRINTF("Failed to close pcsx2_dump.gs\n");
			fail();
			return -1;
		}
		pcsx2_dump_file = NULL;
	}
#elif READ_MODE == READ_MEM
	pcsx2_dump_ptr = NULL;
#endif
	return 0;
}

int my_read_skip(u32 size)
{
#if READ_MODE == READ_USB
	if (fseek(pcsx2_dump_file, size, SEEK_CUR) != 0)
	{
		PRINTF("Failed to skip %d bytes\n", size);
		fail();
		return -1;
	}
#elif READ_MODE == READ_MEM
	pcsx2_dump_ptr += size;
#endif
	return 0;
}

int my_read_pos()
{
#if READ_MODE == READ_USB
	long val = ftell(pcsx2_dump_file);
	if (val < 0)
	{
		PRINTF("Failed to get file position\n");
		fail();
		return -1;
	}
	return (int)val;
#elif READ_MODE == READ_MEM
	return pcsx2_dump_ptr - pcsx2_dump;
#endif
}

int my_read8(u8* value)
{
#if READ_MODE == READ_USB
	if (fread(value, 1, 1, pcsx2_dump_file) != 1)
	{
		PRINTF("Failed to read 1 byte\n");
		fail();
		return -1;
	}
#elif READ_MODE == READ_MEM
  *value = *pcsx2_dump_ptr;
	pcsx2_dump_ptr += 1;
#endif
  return 0;
}

int my_read32(u32* value)
{
#if READ_MODE == READ_USB
	if (fread(value, 1, 4, pcsx2_dump_file) != 4)
	{
		PRINTF("Failed to read 4 bytes\n");
		fail();
		return -1;
	}
#elif READ_MODE == READ_MEM
	*(u8*)value = *(u8*)pcsx2_dump_ptr;
	*((u8*)value + 1) = *((u8*)pcsx2_dump_ptr + 1);
	*((u8*)value + 2) = *((u8*)pcsx2_dump_ptr + 2);
	*((u8*)value + 3) = *((u8*)pcsx2_dump_ptr + 3);
	pcsx2_dump_ptr += 4;
#endif
	return 0;
}

int my_read(u8 *value, u32 size)
{
#if READ_MODE == READ_USB
	if (fread(value, 1, size, pcsx2_dump_file) != size)
	{
		PRINTF("Failed to read %d bytes\n", size);
		fail();
		return -1;
	}
#elif READ_MODE == READ_MEM
  memcpy(value, pcsx2_dump_ptr, size);
	pcsx2_dump_ptr += size;
#endif
  return 0;
}

void gs_transfer(u32 size)
{
  if (size % 16 != 0)
	{
		PRINTF("Transfer size not multiple of 16: %d\n", size);
		fail();
	}

	u32 total_qwc = size / 16;
	while (total_qwc > 0)
	{
    u32 transfer_qwc = total_qwc < TRANSFER_SIZE ? total_qwc : TRANSFER_SIZE;

		if (my_read((u8*)transfer_buffer, transfer_qwc * sizeof(qword_t)) != 0)
		{
			PRINTF("Failed to read transfer data\n");
			fail();
		}

		FlushCache(0); // Flush cache since we copied data to the transfer buffer

		dma_channel_send_normal(DMA_CHANNEL_GIF, transfer_buffer, transfer_qwc, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);

		total_qwc -= transfer_qwc;
	}
	return;
}

int graph_get_mode_from_smode1(u64 smode1)
{
	SMODE1_t* smode1_reg = (SMODE1_t*)&smode1;
	for (int i = 0; i < 22; i++)
	{
		SMODE1_t* smode1_video_mode = (SMODE1_t*)&smode1_values[i];

		// Check only the fields that are are different for different video modes
		if (smode1_reg->RC == smode1_video_mode->RC &&
				smode1_reg->LC == smode1_video_mode->LC &&
				smode1_reg->T1248 == smode1_video_mode->T1248 &&
				smode1_reg->CMOD == smode1_video_mode->CMOD &&
				smode1_reg->PRST == smode1_video_mode->PRST &&
				smode1_reg->PCK2 == smode1_video_mode->PCK2 &&
				smode1_reg->SPML == smode1_video_mode->SPML &&
				smode1_reg->VCKSEL == smode1_video_mode->VCKSEL &&
				smode1_reg->VHP == smode1_video_mode->VHP)
		{
			return i;
		}
	}
	return -1;
}

// Copied from PS2SDK source and modified
int graph_set_mode_custom(int interlace, int mode, int ffmd)
{
	// Reset GS.
	*GS_REG_CSR = (u64)1<<9;

	// Clear GS CSR.
	*GS_REG_CSR = GS_SET_CSR(0,0,0,0,0,0,0,0,0,0,0,0);

	// Unmask GS VSYNC Interrupt.
	GsPutIMR(0x00007700);

	// Ensure registers are written prior to setting another mode.
	asm volatile ("sync.p\n\t"
				  "nop\n\t");

	// Set the requested mode.
	SetGsCrt(interlace, graph_mode[mode].mode, ffmd);

	return 0;
}

void gs_set_privileged(gs_registers_t* registers, u32 init)
{
  if (init)
  {
    int mode = graph_get_mode_from_smode1(registers->SMODE1);

    if (mode == -1)
    {
      PRINTF("Unknown mode\n");
      return;
    }

    u32 interlace = !!(registers->SMODE2 & 1);
    u32 ffmd = !!(registers->SMODE2 & 2);
    graph_set_mode_custom(interlace, mode, ffmd);

		// Note: Should these be set explicitly or are they handled by the graph_set_mode_custom call?
    // *GS_REG_SYNCHV = registers->SYNCV;
    // *GS_REG_SYNCH2 = registers->SYNCH2;
    // *GS_REG_SYNCH1 = registers->SYNCH1;
    // *GS_REG_SRFSH = registers->SRFSH;

    *GS_REG_SMODE2 = registers->SMODE2;
    *GS_REG_PMODE = registers->PMODE;

		// Note: Should this be set or enough to call graph_set_mode_custom?
    // *GS_REG_SMODE1 = registers->SMODE1;
  }

	// CSR and IMR should not be set here
	// They are only used for interrupts, which are not need for the dump

	*GS_REG_BGCOLOR = registers->BGCOLOR;
	*GS_REG_EXTWRITE = registers->EXTWRITE;
	*GS_REG_EXTDATA = registers->EXTDATA;
	*GS_REG_EXTBUF = registers->EXTBUF;

	*GS_REG_DISPFB1 = registers->DISP[0].DISPFB;
	*GS_REG_DISPLAY1 = registers->DISP[0].DISPLAY;
	*GS_REG_DISPFB2 = registers->DISP[1].DISPFB;
	*GS_REG_DISPLAY2 = registers->DISP[1].DISPLAY;
  
	return;
}

#define SET_GS_REG(reg) \
	q->dw[0] = *(u64*)data_ptr; \
	q->dw[1] = reg; \
	q++; \
	data_ptr += sizeof(u64);

void gs_set_state(u8* data_ptr, u32 version)
{
	PRINTF("State version %d\n", version);

	qword_t* reg_packet = aligned_alloc(64, sizeof(qword_t) * 200);
	qword_t* q = reg_packet;

	PACK_GIFTAG(q, GIF_SET_TAG(39, 0, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);

	q++;
	SET_GS_REG(GS_REG_PRIM);

	if (version <= 6)
		data_ptr += sizeof(u64); // PRMODE

	SET_GS_REG(GS_REG_PRMODECONT);
	SET_GS_REG(GS_REG_TEXCLUT);
	SET_GS_REG(GS_REG_SCANMSK);
	SET_GS_REG(GS_REG_TEXA);
	SET_GS_REG(GS_REG_FOGCOL);
	SET_GS_REG(GS_REG_DIMX);
	SET_GS_REG(GS_REG_DTHE);
	SET_GS_REG(GS_REG_COLCLAMP);
	
	SET_GS_REG(GS_REG_PABE);
	SET_GS_REG(GS_REG_BITBLTBUF);
	SET_GS_REG(GS_REG_TRXDIR);
	SET_GS_REG(GS_REG_TRXPOS);
	SET_GS_REG(GS_REG_TRXREG);
	SET_GS_REG(GS_REG_TRXREG);

	SET_GS_REG(GS_REG_XYOFFSET_1);
	*(u64*)data_ptr |= 1; // Reload clut
	SET_GS_REG(GS_REG_TEX0_1);
	SET_GS_REG(GS_REG_TEX1_1);
	if (version <= 6)
		data_ptr += sizeof(u64); // TEX2
	SET_GS_REG(GS_REG_CLAMP_1);
	SET_GS_REG(GS_REG_MIPTBP1_1);
	SET_GS_REG(GS_REG_MIPTBP2_1);
	SET_GS_REG(GS_REG_SCISSOR_1);
	SET_GS_REG(GS_REG_ALPHA_1);
	SET_GS_REG(GS_REG_TEST_1);
	SET_GS_REG(GS_REG_FBA_1);
	SET_GS_REG(GS_REG_FRAME_1);
	SET_GS_REG(GS_REG_ZBUF_1);

	if (version <= 4)
		data_ptr += sizeof(u32) * 7; // skip ???

	SET_GS_REG(GS_REG_XYOFFSET_2);
	*(u64*)data_ptr |= 1; // Reload clut
	SET_GS_REG(GS_REG_TEX0_2);
	SET_GS_REG(GS_REG_TEX1_2);
	if (version <= 6)
		data_ptr += sizeof(u64); // TEX2
	SET_GS_REG(GS_REG_CLAMP_2);
	SET_GS_REG(GS_REG_MIPTBP1_2);
	SET_GS_REG(GS_REG_MIPTBP2_2);
	SET_GS_REG(GS_REG_SCISSOR_2);
	SET_GS_REG(GS_REG_ALPHA_2);
	SET_GS_REG(GS_REG_TEST_2);
	SET_GS_REG(GS_REG_FBA_2);
	SET_GS_REG(GS_REG_FRAME_2);
	SET_GS_REG(GS_REG_ZBUF_2);
	if (version <= 4)
		data_ptr += sizeof(u32) * 7; // skip ???

	PACK_GIFTAG(q, GIF_SET_TAG(5, 1, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	SET_GS_REG(GS_REG_RGBAQ);
	SET_GS_REG(GS_REG_ST);
	// UV
	q->dw[0] = *(u32*)data_ptr;
	q->dw[1] = GS_REG_UV;
	q++;
	data_ptr += sizeof(u32);
	// FOG
	q->sw[0] = *(u32*)data_ptr;
	q->dw[1] = GS_REG_FOG;
	q++;
	data_ptr += sizeof(u32);
	SET_GS_REG(GS_REG_XYZ2);
	qword_t* reg_packet_end = q; // Kick the register packet after the vram upload

	data_ptr += sizeof(u64); // obsolete apparently

	// Unsure how to use these
	data_ptr += sizeof(u32); // m_tr.x
	data_ptr += sizeof(u32); // m_tr.y

	qword_t* vram_packet = aligned_alloc(64, sizeof(qword_t) * 0x50005);

	u8* swizzle_vram = aligned_alloc(64, 0x400000);
	// The current data is 'RAW' vram data, so deswizzle it, so when we upload it
	// the GS with swizzle it back to it's 'RAW' format.
	deswizzleImage(swizzle_vram, data_ptr, 1024 / 64, 1024 / 32);

	q = vram_packet;
	// Set up our registers for the transfer
	PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	q->dw[0] = GS_SET_BITBLTBUF(0, 0, 0, 0, 16, 0); // Page 0, PSMCT32, TBW 1024
	q->dw[1] = GS_REG_BITBLTBUF;
	q++;
	q->dw[0] = GS_SET_TRXPOS(0, 0, 0, 0, 0);
	q->dw[1] = GS_REG_TRXPOS;
	q++;
	q->dw[0] = GS_SET_TRXREG(1024, 1024);
	q->dw[1] = GS_REG_TRXREG;
	q++;
	q->dw[0] = GS_SET_TRXDIR(0);
	q->dw[1] = GS_REG_TRXDIR;
	q++;
	dma_channel_send_normal(DMA_CHANNEL_GIF, vram_packet, q - vram_packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	qword_t* vram_ptr = (qword_t*)swizzle_vram;
	for (int i = 0; i < 16; i++)
	{
		q = vram_packet;
		PACK_GIFTAG(q, GIF_SET_TAG(0x4000, i == 15 ? 1 : 0, 0, 0, 2, 0), 0);
		q++;
		memcpy(q, vram_ptr, 0x4000 * sizeof(qword_t));
		q += 0x4000;
		vram_ptr += 0x4000;

		dma_channel_send_normal(DMA_CHANNEL_GIF, vram_packet, q - vram_packet, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
	}

	// Kick the register packet
	dma_channel_send_normal(DMA_CHANNEL_GIF, reg_packet, reg_packet_end - reg_packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(reg_packet);
	free(vram_packet);
	free(swizzle_vram);
	return;
}

void read_gs_dump_header()
{
  memset(&gs_dump_header, 0, sizeof(gs_dump_header));

	u32 crc;
	my_read32(&crc);
	if (crc == 0xFFFFFFFF)
	{
		u32 header_size;
		my_read32(&header_size);
		my_read32(&gs_dump_header.state_version);
    my_read32(&gs_dump_header.state_size);
    my_read32(&gs_dump_header.serial_offset);
    my_read32(&gs_dump_header.serial_size);
    my_read32(&gs_dump_header.crc);
    my_read32(&gs_dump_header.screenshot_width);
    my_read32(&gs_dump_header.screenshot_height);
    my_read32(&gs_dump_header.screenshot_offset);
    my_read32(&gs_dump_header.screenshot_size);
	}
	else
	{
		gs_dump_header.old = 1;
		my_read32(&gs_dump_header.state_size);
		my_read32(&gs_dump_header.state_version);
		gs_dump_header.crc = crc;
	}
}

void run_gs_dump(u32 loops)
{
	u32 loop = 0;
  while (loop < loops)
  {
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    
		my_read_init();
		
    read_gs_dump_header();
    PRINTF("%08x: Header\n", my_read_pos());

    // Skip over the header data
		my_read_skip(gs_dump_header.serial_size);
		my_read_skip(gs_dump_header.screenshot_size);

		if (loop == 0)
		{
			PRINTF("Dump Header\n");
			PRINTF("  old: %d\n", gs_dump_header.old);
			PRINTF("  state_version: %d\n", gs_dump_header.state_version);
			PRINTF("  state_size: %d\n", gs_dump_header.state_size);
			PRINTF("  serial_offset: %d\n", gs_dump_header.serial_offset);
			PRINTF("  serial_size: %d\n", gs_dump_header.serial_size);
			PRINTF("  crc: %d\n", gs_dump_header.crc);
			PRINTF("  screenshot_width: %d\n", gs_dump_header.screenshot_width);
			PRINTF("  screenshot_height: %d\n", gs_dump_header.screenshot_height);
			PRINTF("  screenshot_offset: %d\n", gs_dump_header.screenshot_offset);
			PRINTF("  screenshot_size: %d\n", gs_dump_header.screenshot_size);
		}

    if (!gs_dump_header.old)
      my_read_skip(sizeof(gs_dump_header.state_version)); // Skip state version

    const u32 state_size = gs_dump_header.state_size - 4;

		if (loop == 0)
		{
			my_read(gs_dump_state, state_size);
		}
		else
		{
			my_read_skip(state_size);
		}

    PRINTF("%08x: State\n", my_read_pos());

		my_read((u8*)&register_buffer, GS_REGISTERS_SIZE);

		PRINTF("%08x: Registers\n", my_read_pos());
		gs_set_privileged(&register_buffer, 1);

    gs_set_state(gs_dump_state, gs_dump_header.state_version);

    u32 vsync = 0;
    while (!my_read_eof())
    {
			u8 tag;
      my_read8(&tag);
      // PRINTF("%08x: Tag %d\n", my_read_pos(), tag);

      switch (tag)
      {
        case GS_DUMP_TRANSFER:
        {
					u8 path;
					u32 size;
          my_read8(&path);
          my_read32(&size);
					
          // PRINTF("%08x: Transfer %d %u\n", my_read_pos(), path, size);
					
          gs_transfer(size);

          break;
        }
        case GS_DUMP_VSYNC:
        {
					
					if (vsync == SAVE_FRAME)
					{
						return;
					}

					u8 field;
					my_read8(&field);

          vsync++;

          PRINTF("%08x: Vsync %d\n", my_read_pos(), field);

          // graph_wait_vsync();
          break;
        }
        case GS_DUMP_FIFO:
        {
					u32 size;
          my_read32(&size);

          PRINTF("%08x: Fifo %d\n", my_read_pos(), size);
          break;
        }
        case GS_DUMP_REGISTERS:
        {
					my_read((u8*)&register_buffer, GS_REGISTERS_SIZE);
						
					PRINTF("%08x: Registers\n", my_read_pos());
					gs_set_privileged(&register_buffer, 0);

          break;
        }
      }
    }
		loop++;
  }
}

int graph_initialize_custom()
{
	int mode = graph_get_region();

	graph_set_mode(GRAPH_MODE_NONINTERLACED, mode, GRAPH_MODE_FRAME, GRAPH_ENABLE);

	graph_set_screen(0, 0, DEBUG_FRAME_WIDTH, DEBUG_FRAME_HEIGHT);

	graph_set_bgcolor(0, 0, 0);

	graph_set_framebuffer(0, g_frame.address, g_frame.width, g_frame.psm, 0, 0);

	graph_set_output(1, 0, 1, 0, 1, 0xFF);

	return 0;
}

void init_gs()
{
	g_frame.width = DEBUG_FRAME_WIDTH;
	g_frame.height = DEBUG_FRAME_HEIGHT;
	g_frame.mask = 0;
	g_frame.psm = GS_PSM_32;
	g_frame.address = graph_vram_allocate(DEBUG_FRAME_WIDTH, DEBUG_FRAME_HEIGHT, g_frame.psm, GRAPH_ALIGN_PAGE);

	g_z.enable = DRAW_DISABLE;
	g_z.mask = 1;
	g_z.method = ZTEST_METHOD_ALLPASS;
	g_z.zsm = GS_ZBUF_32;
	g_z.address = graph_vram_allocate(DEBUG_FRAME_WIDTH, DEBUG_FRAME_HEIGHT, GS_PSMZ_32, GRAPH_ALIGN_PAGE);

	graph_initialize_custom();
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
	PACK_GIFTAG(q, GIF_SET_XYZ(DEBUG_WINDOW_X << 4, DEBUG_WINDOW_Y << 4, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ((DEBUG_WINDOW_X + DEBUG_FRAME_WIDTH) << 4, 0, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ(0, (DEBUG_WINDOW_Y + DEBUG_FRAME_HEIGHT) << 4, 0), GIF_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GIF_SET_XYZ((DEBUG_WINDOW_X + DEBUG_FRAME_WIDTH) << 4, (DEBUG_WINDOW_Y + DEBUG_FRAME_HEIGHT) << 4, 0), GIF_REG_XYZ2);
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

int init_draw()
{
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 1024);
	qword_t* q;

	q = packet;

	q = draw_setup_environment(q, 0, &g_frame, &g_z);

	q = draw_primitive_xyoffset(q, 0, DEBUG_WINDOW_X, DEBUG_WINDOW_Y);

	q = draw_disable_tests(q, 0, &g_z);

	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_wait_fast();

	free(packet);
	return 0;
}

void save_image()
{
  read_framebuffer(SAVE_FRAME_ADDR, SAVE_FRAME_WIDTH / 64, 0, 0, SAVE_FRAME_WIDTH, SAVE_FRAME_HEIGHT, SAVE_FRAME_PSM, frame_data);

  char filename[64];
  
  sprintf(filename, "mass:%s", SAVE_FRAME_NAME);

  if (write_bmp_to_usb(filename, frame_data, SAVE_FRAME_WIDTH, SAVE_FRAME_HEIGHT, SAVE_FRAME_PSM, my_draw_clear_send) != 0)
  {
    PRINTF("Failed to write frame data to USB\n");
  }
}

int main(int argc, char* argv[])
{
	init_gs();
  init_draw();
	if (init_usb(my_draw_clear_send) != 0)
	{
		PRINTF("Failed to initialize USB\n");
		fail();
		return -1;
	}
  run_gs_dump(10000);
	my_read_close();
	init_gs();
  init_draw();
  save_image();
	PRINTF("All done, sleeping\n");
  SleepThread();
  return 0;
}