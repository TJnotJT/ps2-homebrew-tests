
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

#include "gs_dump.h"
#include "my_read.h"
#include "swizzle.h"

#define GS_SET_SMODE1(RC, LC, T1248, SLCK, CMOD, EX, PRST, SINT, XPCK,    \
   PCK2, SPML, GCONT, PHS, PVS, PEHS, PEVS, CLKSEL,    \
   NVCK, SLCK2, VCKSEL, VHP)                           \
 ((u64)((RC)&0x00000007) << 0 | (u64)((LC)&0x0000007F) << 3 |          \
 (u64)((T1248)&0x00000003) << 10 | (u64)((SLCK)&0x00000001) << 12 |   \
 (u64)((CMOD)&0x00000003) << 13 | (u64)((EX)&0x00000001) << 15 |      \
 (u64)((PRST)&0x00000001) << 16 | (u64)((SINT)&0x00000001) << 17 |    \
 (u64)((XPCK)&0x00000001) << 18 | (u64)((PCK2)&0x00000003) << 19 |    \
 (u64)((SPML)&0x0000000F) << 21 | (u64)((GCONT)&0x00000001) << 25 |   \
 (u64)((PHS)&0x00000001) << 26 | (u64)((PVS)&0x00000001) << 27 |      \
 (u64)((PEHS)&0x00000001) << 28 | (u64)((PEVS)&0x00000001) << 29 |    \
 (u64)((CLKSEL)&0x00000003) << 30 | (u64)((NVCK)&0x00000001) << 32 |  \
 (u64)((SLCK2)&0x00000001) << 33 | (u64)((VCKSEL)&0x00000003) << 34 | \
 (u64)((VHP)&0x00000003) << 36)

typedef struct {
  u64 RC : 3; //
  u64 LC : 7; //
  u64 T1248 : 2; //
  u64 SLCK : 1; //
  u64 CMOD : 2; //
  u64 EX : 1; //
  u64 PRST : 1; //
  u64 SINT : 1; //
  u64 XPCK : 1; //
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

// GS_SET_SMODE1(4,  32,  1, 0, 2, 0, 1, 1, 0, 0,  4, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0), //0x02
// GS_SET_SMODE1(4,  32,  1, 0, 3, 0, 1, 1, 0, 0,  4, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0), //0x03
// GS_SET_SMODE1(4,  32,  1, 0, 0, 0, 1, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1), //0x50
// GS_SET_SMODE1(4,  32,  1, 0, 0, 0, 1, 1, 0, 1,  2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1), //0x53
// GS_SET_SMODE1(2,  22,  1, 0, 0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x52
// GS_SET_SMODE1(2,  22,  1, 0, 0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0), //0x51
// GS_SET_SMODE1(2,  15,  1, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x1A
// GS_SET_SMODE1(3,  28,  1, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x1B
// GS_SET_SMODE1(3,  28,  1, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x1C
// GS_SET_SMODE1(3,  16,  0, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x1D
// GS_SET_SMODE1(3,  16,  0, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x2A
// GS_SET_SMODE1(6,  71,  1, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x2B
// GS_SET_SMODE1(5,  74,  1, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x2C
// GS_SET_SMODE1(3,  44,  1, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x2D
// GS_SET_SMODE1(3,  25,  0, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x2E
// GS_SET_SMODE1(3,  29,  0, 0, 0, 0, 0, 1, 0, 0,  2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x3B
// GS_SET_SMODE1(6,  67,  1, 0, 0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x3C
// GS_SET_SMODE1(3,  35,  1, 0, 0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x3D
// GS_SET_SMODE1(1,   7,  0, 0, 0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x3E
// GS_SET_SMODE1(1,   8,  0, 0, 0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x4A
// GS_SET_SMODE1(1,  10,  0, 0, 0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1), //0x4B
// GS_SET_SMODE1(7, 127,  3, 0, 3, 0, 1, 0, 0, 1, 15, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3), //MASK

gs_dump_t dump __attribute__((aligned(16)));
qword_t temp_buffer[16] __attribute__((aligned(16)));
gs_registers privileged_registers __attribute__((aligned(16)));

extern GRAPH_MODE graph_mode[22];

extern u64 smode1_values[22];

void gs_transfer(u8* packet, u32 size)
{
  assert(size % 16 == 0);

	volatile u32* GIFCHCR = ((volatile u32*)0x1000A000);
	volatile u32* GIFMADR = ((volatile u32*)0x1000A010);
	volatile u32* GIFQWC = ((volatile u32*)0x1000A020);

	u32 transfer_cnt = size / 16;
	do
	{
    u32 transfer = transfer_cnt < 0xF000 ? transfer_cnt : 0xF000;
    memcpy(temp_buffer, packet, transfer * sizeof(qword_t));

    // TODO: Replace with dma_channel_send_normal
	  *GIFMADR = (u32)temp_buffer;
		*GIFQWC = transfer;
		transfer_cnt -= transfer;
		FlushCache(0);
		*GIFCHCR = 0x100;
		while (*GIFCHCR & 0x100)
		{
		}

	} while (transfer_cnt > 0);
	return;
}



void gs_set_privileged(const u8* data, u32 init)
{
  memcpy(&privileged_registers, data, sizeof(gs_registers));

  if (init)
  {
    // int mode = graph_get_region();
    int mode = -1;

    for (int i = 0; i < 22; i++)
    {
      u64 mask = GS_SET_SMODE1(7, 127,  3, 0, 3, 0, 1, 0, 0, 1, 15, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3);
      u64 smode1_masked = privileged_registers.SMODE1 & mask;
      u64 smode1_values_masked = smode1_values[i] & mask;
      SMODE1_t* smode1 = (SMODE1_t*)&privileged_registers.SMODE1;
      SMODE1_t* smode1_value = (SMODE1_t*)&smode1_values[i];
      printf("SMODE1 %d RC=%x, LC=%x, T1248=%x, SLCK=%x, CMOD=%x, EX=%x, PRST=%x, SINT=%x, XPCK=%x, PCK2=%x, SPML=%x, GCONT=%x, PHS=%x, PVS=%x, PEHS=%x, PEVS=%x, CLKSEL=%x, NVCK=%x, SLCK2=%x, VCKSEL=%x, VHP=%x\n", i, smode1->RC, smode1->LC, smode1->T1248, smode1->SLCK, smode1->CMOD, smode1->EX, smode1->PRST, smode1->SINT, smode1->XPCK, smode1->PCK2, smode1->SPML, smode1->GCONT, smode1->PHS, smode1->PVS, smode1->PEHS, smode1->PEVS, smode1->CLKSEL, smode1->NVCK, smode1->SLCK2, smode1->VCKSEL, smode1->VHP);
      printf("SMODE1 %d RC=%x, LC=%x, T1248=%x, SLCK=%x, CMOD=%x, EX=%x, PRST=%x, SINT=%x, XPCK=%x, PCK2=%x, SPML=%x, GCONT=%x, PHS=%x, PVS=%x, PEHS=%x, PEVS=%x, CLKSEL=%x, NVCK=%x, SLCK2=%x, VCKSEL=%x, VHP=%x\n", i, smode1_value->RC, smode1_value->LC, smode1_value->T1248, smode1_value->SLCK, smode1_value->CMOD, smode1_value->EX, smode1_value->PRST, smode1_value->SINT, smode1_value->XPCK, smode1_value->PCK2, smode1_value->SPML, smode1_value->GCONT, smode1_value->PHS, smode1_value->PVS, smode1_value->PEHS, smode1_value->PEVS, smode1_value->CLKSEL, smode1_value->NVCK, smode1_value->SLCK2, smode1_value->VCKSEL, smode1_value->VHP);
      printf("--\n");
      // printf("SMODE1 %d %08llx %08llx\n", i, smode1_values[i], privileged_registers.SMODE1);
      if (smode1_masked == smode1_values_masked)
      {
        mode = i;
        printf("Mode %d\n", mode);
        break;
      }
    }

    if (mode == -1)
    {
      printf("Unknown mode\n");
      return;
    }

    // TODO: Infer the mode and field/flicker_filter from the registers
    // This function modifies CSR and IMR so we chould restore them after
    u32 interlace = !!(privileged_registers.SMODE2 & 1);
    u32 ffmd = !!(privileged_registers.SMODE2 & 2);
    graph_set_mode(interlace, mode, ffmd, GRAPH_ENABLE);

    *GS_REG_SYNCHV = privileged_registers.SYNCV;
    *GS_REG_SYNCH2 = privileged_registers.SYNCH2;
    *GS_REG_SYNCH1 = privileged_registers.SYNCH1;
    *GS_REG_SRFSH = privileged_registers.SRFSH;

    *GS_REG_SMODE2 = privileged_registers.SMODE2;
    *GS_REG_PMODE = privileged_registers.PMODE;
    *GS_REG_SMODE1 = privileged_registers.SMODE1;
  }

	*GS_REG_BGCOLOR = privileged_registers.BGCOLOR;
	*GS_REG_EXTWRITE = privileged_registers.EXTWRITE;
	*GS_REG_EXTDATA = privileged_registers.EXTDATA;
	*GS_REG_EXTBUF = privileged_registers.EXTBUF;
	*GS_REG_DISPFB1 = privileged_registers.DISP[0].DISPFB;
	*GS_REG_DISPLAY1 = privileged_registers.DISP[0].DISPLAY;
	*GS_REG_DISPFB2 = privileged_registers.DISP[1].DISPFB;
	*GS_REG_DISPLAY2 = privileged_registers.DISP[1].DISPLAY;

  *GS_REG_CSR = privileged_registers.CSR;
  *GS_REG_IMR = privileged_registers.IMR;
  
	return;
}

#define SET_GS_REG(reg) \
	q->dw[0] = *(u64*)data_ptr; \
	q->dw[1] = reg; \
	q++; \
	data_ptr += sizeof(u64);

void gs_set_state(u8* data_ptr, u32 version)
{
	printf("State version %d\n", version);

	qword_t* reg_packet = aligned_alloc(64, sizeof(qword_t) * 200);
	qword_t* q = reg_packet;

	PACK_GIFTAG(q, GIF_SET_TAG(3, 0, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 13),
		GIF_REG_AD | (GIF_REG_AD << 4) | (GIF_REG_AD << 8) | (GIF_REG_AD << 12) | (GIF_REG_AD << 16) |
			(GIF_REG_AD << 20) | (GIF_REG_AD << 24) | ((u64)GIF_REG_AD << 28) | ((u64)GIF_REG_AD << 32) |
			((u64)GIF_REG_AD << 36) | ((u64)GIF_REG_AD << 40) | ((u64)GIF_REG_AD << 44) | ((u64)GIF_REG_AD << 48));

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
	;
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

	PACK_GIFTAG(q, GIF_SET_TAG(5, 1, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1),
		GIF_REG_AD);
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

	q = vram_packet;
	qword_t* vram_ptr = (qword_t*)swizzle_vram;
	for (int i = 0; i < 16; i++)
	{
		// Idea, low priority -- this is done once:
		// Upload the GIFTAG
		// Send it down the DMA
		// Point the DMA at the vram ptr
		// Upload that QWC
		// Repeat
		q = vram_packet;
		PACK_GIFTAG(q, GIF_SET_TAG(0x4000, i == 15 ? 1 : 0, 0, 0, 2, 0), 0);
		q++;
		for (int j = 0; j < 0x4000; j++)
		{
			*q = *(qword_t*)vram_ptr;
			vram_ptr++;
			q++;
		}

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
		memset(&dump.header, 0, sizeof(dump.header));
		dump.header.old = 1;
		data = my_read32(data, &dump.header.state_size);
		data = my_read32(data, &dump.header.state_version);
		dump.header.crc = crc;
	}
  return data;
}

void read_gs_dump(const u8* data, const u8* const data_end)
{
  u8* data_start = (u8*)data;

  data = read_gs_dump_header(data);
  printf("%08x: Header\n", data - data_start);

  // Skip over the header data
  data += dump.header.serial_size;
  data += dump.header.screenshot_size;

  if (!dump.header.old)
    data += sizeof(dump.header.state_version); // Skip state version

  u32 state_size = dump.header.state_size - 4;
  data = my_read_ptr(data, (const void**)&dump.state, state_size);
  printf("%08x: State\n", data - data_start);
  gs_set_state(dump.state, dump.header.state_version);

  data = my_read_ptr(data, (const void**)&dump.registers, REGISTERS_SIZE);
  printf("%08x: Registers\n", data - data_start);
  gs_set_privileged(dump.registers, 1);

  gs_dump_command_t* curr_command = dump.commands;
  
  while (data < data_end)
  {
    memset(curr_command, 0, sizeof(gs_dump_command_t));

    data = my_read8(data, &curr_command->tag);
    printf("%08x: Tag %d\n", data - data_start, curr_command->tag);
    switch (curr_command->tag)
    {
      case GS_DUMP_TRANSFER:
      {
        data = my_read8(data, &curr_command->path);
        data = my_read32(data, &curr_command->size);
        data = my_read_ptr(data, (const void**)&curr_command->data, curr_command->size);

        printf("%08x: Transfer %d %d\n", data - data_start, curr_command->path, curr_command->size);
        
        gs_transfer((u8*)curr_command->data, curr_command->size);
        break;
      }
      case GS_DUMP_VSYNC:
      {
        data = my_read8(data, &curr_command->field);

        printf("%08x: Vsync %d\n", data - data_start, curr_command->field);

        draw_wait_finish();
        graph_wait_vsync();
        break;
      }
      case GS_DUMP_FIFO:
      {
        data = my_read32(data, &curr_command->size);

        printf("%08x: Fifo %d\n", data - data_start, curr_command->size);
        break;
      }
      case GS_DUMP_REGISTERS:
      {
        data = my_read_ptr(data, (const void**)&curr_command->data, 8192);

        gs_set_privileged(curr_command->data, 0);

        printf("%08x: Registers\n", data - data_start);
        break;
      }
    }
  }
}

extern u32 size_pcsx2_dump;
extern u8 pcsx2_dump[] __attribute__((aligned(16)));

int main(int argc, char* argv[])
{
  dma_channel_initialize(DMA_CHANNEL_GIF,NULL,0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);


  read_gs_dump(pcsx2_dump, pcsx2_dump + size_pcsx2_dump);
  return 0;
}