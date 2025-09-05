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
#include <assert.h>

#include "read-fb.h"

#include "../lib-common/common.h"

// Taken from gs4ps2.c (https://github.com/F0bes/gs4ps2)
void gs_glue_read_fifo(u32 QWtotal, u128* data)
{
	volatile u32* VIF1CHCR = (u32*)0x10009000;
	volatile u32* VIF1MADR = (u32*)0x10009010;
	volatile u32* VIF1QWC = (u32*)0x10009020;
	volatile u128* VIF1FIFO = (u128*)0x10005000;
	volatile u32* VIF1_STAT = (u32*)0x10003C00;

	*VIF1_STAT = (1 << 23); // Set the VIF FIFO direction to VIF1 -> Main Memory
	*GS_REG_BUSDIR = 1;

	u32 QWrem = QWtotal;
	u128* data_curr = data;

	printf("Doing a readback of %d QW", QWtotal);
	while (QWrem >= 8) // Data transfers from the FIFO must be 128 byte aligned
	{
		*VIF1MADR = (u32)data_curr;
		u32 QWC = (((0xF000) < QWrem) ? (0xF000) : QWrem);
		QWC &= ~7;

		*VIF1QWC = QWC;
		QWrem -= QWC;
		data_curr += QWC;

		printf("    Reading chunk of %d QW. (Remaining %d)", QWC, QWrem);
		FlushCache(0);
		*VIF1CHCR = 0x100;
		asm __volatile__(" sync.l\n");
		while (*VIF1CHCR & 0x100)
		{
			//printf("VIF1CHCR %X\nVIF1STAT %X\nVIF1QWC %X\nGIF_STAT %X\n"
			//,*VIF1CHCR,*VIF1_STAT, *VIF1QWC, *(volatile u64*)0x10003020);
		};
	}
	printf("Finished off DMAC transfer, manually reading the %d QW from the fifo", QWrem);
	// Because we truncate the QWC, finish of the rest of the transfer
	// by reading the FIFO directly. Apparently this is how retail games do it.
	u32 qwFlushed = 0;
	while ((*VIF1_STAT >> 24))
	{
		printf("VIF1_STAT=%x", *VIF1_STAT);
		*data_curr = *VIF1FIFO;
		QWrem++;
		qwFlushed += 4;
		asm("nop\nnop\nnop\n");
	}
	printf("Finished off the manual read (%d QW)\n", qwFlushed);
	*GS_REG_BUSDIR = 0; // Reset BUSDIR
	*VIF1_STAT = 0; // Reset FIFO direction

	printf("Resetting TRXDIR");
	// Create a data packet that sets TRXDIR to 3, effectively cancelling whatever
	// transfer may be going on.
	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 2);
	qword_t* q = packet;
	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	q->dw[0] = GS_SET_TRXDIR(3);
	q->dw[1] = GS_REG_TRXDIR;
	q++;
	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	free(packet);
}

// Taken from gs4ps2.c (https://github.com/F0bes/gs4ps2)
void read_framebuffer(u32 address, u32 bw, u32 x, u32 y, u32 width, u32 height, u32 psm, void* out_data)
{
	*GS_REG_CSR = 2; // Clear any previous FINISH events

	qword_t* packet = aligned_alloc(64, sizeof(qword_t) * 16);
	qword_t* q = packet;
	PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_FINISH(1), GS_REG_FINISH);
	q++;
	PACK_GIFTAG(q, GS_SET_BITBLTBUF(address >> 6, bw, psm, 0, 0, 0), GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXPOS(x, y, 0, 0, 0), GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXREG(width, height), GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q, GS_SET_FINISH(1), GS_REG_FINISH);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXDIR(1), GS_REG_TRXDIR);
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet, q - packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	while (!(*GS_REG_CSR & 2))
		;

  u32 QWC = width * height * gs_psm_bpp(psm) / 128; // Calculate the number of QW to read

	gs_glue_read_fifo(QWC, (u128*)out_data);

	return;
}
