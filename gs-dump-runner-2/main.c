#include <kernel.h>
#include <stdlib.h>
#include <malloc.h>
#include <tamtypes.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <gs_privileged.h>
#include <gs_gp.h>
#include <graph.h>
#include <dma.h>
#include <draw.h>

#include "main.h"
#include "my_read.h"
#include "swizzle.h"

#include "../lib-usb/usb.h"
#include "../lib-bmp/bmp.h"
#include "../lib-read-fb/read-fb.h"

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

gs_dump_t dump __attribute__((aligned(16))); // For storing the GS dump
qword_t transfer_buffer[TRANSFER_SIZE] __attribute__((aligned(16))); // For transferring GS packets with qword alignment
gs_registers_t register_buffer __attribute__((aligned(16))); // For transferring GS privileged registers with qword alignment
u8 gs_dump_state[0x401000] __attribute__((aligned(16)));

extern GRAPH_MODE graph_mode[22]; // For inferring the video mode
extern u64 smode1_values[22]; // For inferring the video mode

void gs_transfer(u8* packet, u32 size)
{
  assert(size % 16 == 0);

	u32 total_qwc = size / 16;
	while (total_qwc > 0)
	{
    u32 transfer_qwc = total_qwc < TRANSFER_SIZE ? total_qwc : TRANSFER_SIZE;
    memcpy(transfer_buffer, packet, transfer_qwc * sizeof(qword_t));

		FlushCache(0); // Flush cache since we copied data to the transfer buffer

		dma_channel_send_normal(DMA_CHANNEL_GIF, transfer_buffer, transfer_qwc, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);

		total_qwc -= transfer_qwc;
		packet += transfer_qwc * sizeof(qword_t);
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

void gs_set_privileged(const u8* data, u32 init)
{
  memcpy(&register_buffer, data, sizeof(gs_registers_t));

  if (init)
  {
    int mode = graph_get_mode_from_smode1(register_buffer.SMODE1);

    if (mode == -1)
    {
      PRINTF("Unknown mode\n");
      return;
    }

    u32 interlace = !!(register_buffer.SMODE2 & 1);
    u32 ffmd = !!(register_buffer.SMODE2 & 2);
    graph_set_mode_custom(interlace, mode, ffmd);

		// Note: Should these be set explicitly or or they handled by the graph_set_mode_custom call?
    // *GS_REG_SYNCHV = register_buffer.SYNCV;
    // *GS_REG_SYNCH2 = register_buffer.SYNCH2;
    // *GS_REG_SYNCH1 = register_buffer.SYNCH1;
    // *GS_REG_SRFSH = register_buffer.SRFSH;

    *GS_REG_SMODE2 = register_buffer.SMODE2;
    *GS_REG_PMODE = register_buffer.PMODE;

		// Note: Should this be set or enough to call graph_set_mode_custom?
    // *GS_REG_SMODE1 = register_buffer.SMODE1;
  }

	// CSR and IMR should not be set here
	// They are only used for interrupts, which are not need for the dump

	*GS_REG_BGCOLOR = register_buffer.BGCOLOR;
	*GS_REG_EXTWRITE = register_buffer.EXTWRITE;
	*GS_REG_EXTDATA = register_buffer.EXTDATA;
	*GS_REG_EXTBUF = register_buffer.EXTBUF;

	*GS_REG_DISPFB1 = register_buffer.DISP[0].DISPFB;
	*GS_REG_DISPLAY1 = register_buffer.DISP[0].DISPLAY;
	*GS_REG_DISPFB2 = register_buffer.DISP[1].DISPFB;
	*GS_REG_DISPLAY2 = register_buffer.DISP[1].DISPLAY;
  
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

const u8* read_gs_dump_header(const u8* data)
{
  memset(&dump.header, 0, sizeof(dump.header));

	u32 crc;
	data = my_read32(data, &crc);
	if (crc == 0xFFFFFFFF)
	{
		u32 header_size;
		data = my_read32(data, &header_size);
		data = my_read32(data, &dump.header.state_version);
    data = my_read32(data, &dump.header.state_size);
    data = my_read32(data, &dump.header.serial_offset);
    data = my_read32(data, &dump.header.serial_size);
    data = my_read32(data, &dump.header.crc);
    data = my_read32(data, &dump.header.screenshot_width);
    data = my_read32(data, &dump.header.screenshot_height);
    data = my_read32(data, &dump.header.screenshot_offset);
    data = my_read32(data, &dump.header.screenshot_size);
	}
	else
	{
		dump.header.old = 1;
		data = my_read32(data, &dump.header.state_size);
		data = my_read32(data, &dump.header.state_version);
		dump.header.crc = crc;
	}
  return data;
}

void run_gs_dump(const u8* data, const u8* const data_end, u32 loops)
{
  const u8* const data_start = (const u8*)data;

  memset(&dump, 0, sizeof(dump));

	u32 loop = 0;
  while (loop < loops)
  {
    dma_channel_initialize(DMA_CHANNEL_GIF,NULL,0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    data = data_start;
    data = read_gs_dump_header(data);
    PRINTF("%08x: Header\n", data - data_start);

    // Skip over the header data
    data += dump.header.serial_size;
    data += dump.header.screenshot_size;

		if (loop == 0)
		{
			PRINTF("Dump Header\n");
			PRINTF("  old: %d\n", dump.header.old);
			PRINTF("  state_version: %d\n", dump.header.state_version);
			PRINTF("  state_size: %d\n", dump.header.state_size);
			PRINTF("  serial_offset: %d\n", dump.header.serial_offset);
			PRINTF("  serial_size: %d\n", dump.header.serial_size);
			PRINTF("  crc: %d\n", dump.header.crc);
			PRINTF("  screenshot_width: %d\n", dump.header.screenshot_width);
			PRINTF("  screenshot_height: %d\n", dump.header.screenshot_height);
			PRINTF("  screenshot_offset: %d\n", dump.header.screenshot_offset);
			PRINTF("  screenshot_size: %d\n", dump.header.screenshot_size);
		}

    if (!dump.header.old)
      data += sizeof(dump.header.state_version); // Skip state version

    const u32 state_size = dump.header.state_size - 4;

		if (loop == 0)
		{
			data = my_read(data, gs_dump_state, state_size);
		}
		else
		{
			data += state_size;
		}
		
    PRINTF("%08x: State\n", data - data_start);
    
		data = my_read_ptr(data, (const void**)&dump.registers, REGISTERS_SIZE);

		PRINTF("%08x: Registers\n", data - data_start);
		gs_set_privileged(dump.registers, 1);

    gs_set_state(gs_dump_state, dump.header.state_version);

    gs_dump_command_t* curr_command = malloc(sizeof(gs_dump_command_t));
  
    u32 vsync = 0;
    while (data < data_end)
    {
      memset(curr_command, 0, sizeof(gs_dump_command_t));

      data = my_read8(data, &curr_command->tag);
      // PRINTF("%08x: Tag %d\n", data - data_start, curr_command->tag);

      switch (curr_command->tag)
      {
        case GS_DUMP_TRANSFER:
        {
          data = my_read8(data, &curr_command->path);
          data = my_read32(data, &curr_command->size);
          data = my_read_ptr(data, (const void**)&curr_command->data, curr_command->size);

          // PRINTF("%08x: Transfer %d %d\n", data - data_start, curr_command->path, curr_command->size);
          
          gs_transfer((u8*)curr_command->data, curr_command->size);
          break;
        }
        case GS_DUMP_VSYNC:
        {
					
					if (loop > 0 && vsync == SAVE_FRAME)
					{
						free(curr_command);
						return;
					}

					data = my_read8(data, &curr_command->field);

          vsync++;

          PRINTF("%08x: Vsync %d\n", data - data_start, curr_command->field);

          // graph_wait_vsync();
          break;
        }
        case GS_DUMP_FIFO:
        {
          data = my_read32(data, &curr_command->size);

          PRINTF("%08x: Fifo %d\n", data - data_start, curr_command->size);
          break;
        }
        case GS_DUMP_REGISTERS:
        {

					data = my_read_ptr(data, (const void**)&curr_command->data, 8192);
						
					PRINTF("%08x: Registers\n", data - data_start);
					gs_set_privileged(curr_command->data, 0);

          break;
        }
      }
    }

    free(curr_command);
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

extern u32 size_pcsx2_dump;
extern u8 pcsx2_dump[] __attribute__((aligned(16)));

int main(int argc, char* argv[])
{
  run_gs_dump(pcsx2_dump, pcsx2_dump + size_pcsx2_dump, 10000);
	init_gs();
  init_draw();
  save_image();
  SleepThread();
  return 0;
}