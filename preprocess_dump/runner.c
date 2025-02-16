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

#include "common.h"

#define TRANSFER_SIZE 0xFFFF

#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
// #define PRINTF(fmt, ...)

#include "pcsx2_dump_vars.c"

extern GRAPH_MODE graph_mode[22]; // For inferring the video mode
extern u64 smode1_values[22]; // For inferring the video mode

// Transfer data to the GS via DMA GIF channel
void gs_transfer(u8* data, u32 size)
{
  assert((u32)data % 16 == 0);
  assert(size % 16 == 0);

  u32 curr_size = 0;
  while (curr_size < size)
  {
    u32 transfer_size = size < MAX_DMA_SIZE ? size : MAX_DMA_SIZE;

    dma_channel_send_normal(DMA_CHANNEL_GIF, &data[curr_size], transfer_size / 16, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 0);

    curr_size += transfer_size;
  }
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
  *GS_REG_CSR = (u64)(1 << 9);

  // Clear GS CSR.
  *GS_REG_CSR = GS_SET_CSR(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  // Unmask GS VSYNC Interrupt.
  GsPutIMR(0x00007700);

  // Ensure registers are written prior to setting another mode.
  asm volatile ("sync.p\n\t" "nop\n\t");

  // Set the requested mode.
  SetGsCrt(interlace, graph_mode[mode].mode, ffmd);

  return 0;
}

void gs_set_priv_regs(priv_regs_out_t* data, u32 init)
{
  assert((u32)data % 16 == 0);

  if (init)
  {
    int mode = graph_get_mode_from_smode1(data->SMODE1);

    if (mode == -1)
    {
      PRINTF("Unknown mode\n");
      return;
    }

    u32 interlace = !!(data->SMODE2 & 1);
    u32 ffmd = !!(data->SMODE2 & 2);
    graph_set_mode_custom(interlace, mode, ffmd);

    // FIXME: Should these be set?
    // *GS_REG_SYNCHV = data->SYNCV;
    // *GS_REG_SYNCH2 = data->SYNCH2;
    // *GS_REG_SYNCH1 = data->SYNCH1;
    // *GS_REG_SRFSH = data->SRFSH;

    *GS_REG_SMODE2 = data->SMODE2;
    *GS_REG_PMODE = data->PMODE;

    // FIXME: Should this be set or enough to call graph_set_mode_custom?
    // *GS_REG_SMODE1 = register_buffer.SMODE1;
  }

  // CSR and IMR should not be set here
  // They are only used for interrupts, which are not need for the dump

  *GS_REG_BGCOLOR = data->BGCOLOR;
  *GS_REG_EXTWRITE = data->EXTWRITE;
  *GS_REG_EXTDATA = data->EXTDATA;
  *GS_REG_EXTBUF = data->EXTBUF;

  *GS_REG_DISPFB1 = data->DISP[0].DISPFB;
  *GS_REG_DISPLAY1 = data->DISP[0].DISPLAY;
  *GS_REG_DISPFB2 = data->DISP[1].DISPFB;
  *GS_REG_DISPLAY2 = data->DISP[1].DISPLAY; 
}

void gs_set_memory(u8* data)
{
  assert((u32)data % 16 == 0);

  qword_t* vram_packet = aligned_alloc(64, sizeof(qword_t) * 0x50005);
  qword_t* q = vram_packet;

  // Set up our registers for the transfer
  PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
  PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, 0, 16, 0), GS_REG_BITBLTBUF); q++;
  PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS); q++;
  PACK_GIFTAG(q, GS_SET_TRXREG(1024, 1024), GS_REG_TRXREG); q++;
  PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR); q++;

  dma_channel_send_normal(DMA_CHANNEL_GIF, vram_packet, q - vram_packet, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 0);

  // TODO: REPLACE THIS WITH SOURCE CHAIN DMA
  q = vram_packet;
  qword_t* vram_ptr = (qword_t*)data;
  for (int i = 0; i < 16; i++)
  {
    q = vram_packet;
    PACK_GIFTAG(q, GIF_SET_TAG(0x4000, i == 15 ? 1 : 0, 0, 0, 2, 0), 0); q++;
    memcpy(q, vram_ptr, 0x4000 * sizeof(qword_t));
    q += 0x4000;
    vram_ptr += 0x4000;

    dma_channel_send_normal(DMA_CHANNEL_GIF, vram_packet, q - vram_packet, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 0);
  }

  free(vram_packet);
}

void gs_set_general_regs(general_regs_t* data)
{
  assert((u32)data % 16 == 0);
  
  qword_t* reg_packet = aligned_alloc(64, sizeof(qword_t) * 200);
  qword_t* q = reg_packet;

  PACK_GIFTAG(q, GIF_SET_TAG(39, 0, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
  PACK_GIFTAG(q, data->PRIM, GS_REG_PRIM); q++;
  PACK_GIFTAG(q, data->PRMODECONT, GS_REG_PRMODECONT); q++;
  PACK_GIFTAG(q, data->TEXCLUT, GS_REG_TEXCLUT); q++;
  PACK_GIFTAG(q, data->SCANMSK, GS_REG_SCANMSK); q++;
  PACK_GIFTAG(q, data->TEXA, GS_REG_TEXA); q++;
  PACK_GIFTAG(q, data->FOGCOL, GS_REG_FOGCOL); q++;
  PACK_GIFTAG(q, data->DIMX, GS_REG_DIMX); q++;
  PACK_GIFTAG(q, data->DTHE, GS_REG_DTHE); q++;
  PACK_GIFTAG(q, data->COLCLAMP, GS_REG_COLCLAMP); q++;
  PACK_GIFTAG(q, data->PABE, GS_REG_PABE); q++;
  PACK_GIFTAG(q, data->BITBLTBUF, GS_REG_BITBLTBUF); q++;
  PACK_GIFTAG(q, data->TRXDIR, GS_REG_TRXDIR); q++;
  PACK_GIFTAG(q, data->TRXPOS, GS_REG_TRXPOS); q++;
  PACK_GIFTAG(q, data->TRXREG_1, GS_REG_TRXREG); q++;
  PACK_GIFTAG(q, data->TRXREG_2, GS_REG_TRXREG); q++;
  PACK_GIFTAG(q, data->XYOFFSET_1, GS_REG_XYOFFSET_1); q++;
  PACK_GIFTAG(q, ((u64)1 << 61) | (data->TEX0_1 & (((u64)1 << 61) - 1)), GS_REG_TEX0_1); q++; // Set CLD bit 61 to reload the clut
  PACK_GIFTAG(q, data->TEX1_1, GS_REG_TEX1_1); q++;
  PACK_GIFTAG(q, data->CLAMP_1, GS_REG_CLAMP_1); q++;
  PACK_GIFTAG(q, data->MIPTBP1_1, GS_REG_MIPTBP1_1); q++;
  PACK_GIFTAG(q, data->MIPTBP2_1, GS_REG_MIPTBP2_1); q++;
  PACK_GIFTAG(q, data->SCISSOR_1, GS_REG_SCISSOR_1); q++;
  PACK_GIFTAG(q, data->ALPHA_1, GS_REG_ALPHA_1); q++;
  PACK_GIFTAG(q, data->TEST_1, GS_REG_TEST_1); q++;
  PACK_GIFTAG(q, data->FBA_1, GS_REG_FBA_1); q++;
  PACK_GIFTAG(q, data->FRAME_1, GS_REG_FRAME_1); q++;
  PACK_GIFTAG(q, data->ZBUF_1, GS_REG_ZBUF_1); q++;
  PACK_GIFTAG(q, data->XYOFFSET_2, GS_REG_XYOFFSET_2); q++;
  PACK_GIFTAG(q, ((u64)1 << 61) | (data->TEX0_2 & (((u64)1 << 61) - 1)), GS_REG_TEX0_2); q++; // Set CLD bit 61 to reload the clut
  PACK_GIFTAG(q, data->TEX1_2, GS_REG_TEX1_2); q++;
  PACK_GIFTAG(q, data->CLAMP_2, GS_REG_CLAMP_2); q++;
  PACK_GIFTAG(q, data->MIPTBP1_2, GS_REG_MIPTBP1_2); q++;
  PACK_GIFTAG(q, data->MIPTBP2_2, GS_REG_MIPTBP2_2); q++;
  PACK_GIFTAG(q, data->SCISSOR_2, GS_REG_SCISSOR_2); q++;
  PACK_GIFTAG(q, data->ALPHA_2, GS_REG_ALPHA_2); q++;
  PACK_GIFTAG(q, data->TEST_2, GS_REG_TEST_2); q++;
  PACK_GIFTAG(q, data->FBA_2, GS_REG_FBA_2); q++;
  PACK_GIFTAG(q, data->FRAME_2, GS_REG_FRAME_2); q++;
  PACK_GIFTAG(q, data->ZBUF_2, GS_REG_ZBUF_2); q++;

  PACK_GIFTAG(q, GIF_SET_TAG(5, 1, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
  PACK_GIFTAG(q, data->RGBAQ, GS_REG_RGBAQ); q++;
  PACK_GIFTAG(q, data->ST, GS_REG_ST); q++;
  PACK_GIFTAG(q, data->UV, GS_REG_UV); q++;
  PACK_GIFTAG(q, data->FOG, GS_REG_FOG); q++;
  PACK_GIFTAG(q, data->XYZ2, GS_REG_XYZ2); q++;

  dma_channel_send_normal(DMA_CHANNEL_GIF, reg_packet, q - reg_packet, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 0);
  free(reg_packet);
}

void wait_for_vsync_field(u32 field)
{
  int timeout = 0;
  while (((*GS_REG_CSR >> 13) & 1) != field)
  {
    if (timeout % 1000000 == 0)
      PRINTF("Waiting for correct field %d\n", field);
    timeout++;
  }
  if (timeout > 0)
  {
    PRINTF("Field timeout: %d\n", timeout);
  }
}


void exec_gs_dump(s32 loops)
{
  PRINTF("Executing GS dump\n");

  if (loops < 0)
    loops = 0x7FFFFFFF;

  while (loops-- != 0)
  {
    PRINTF("Loop %d\n", loops);
    
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    gs_set_memory(memory_init);
    gs_set_general_regs(&general_regs_init);
    gs_set_priv_regs(&priv_regs_init, 1);

    u32 vsync_n = 0;

    // Set the register for the first Vsync
    if (vsync_count > 0)
    {
      gs_set_priv_regs((priv_regs_out_t*)&command_data[commands[vsync_reg_pos[0]].command_data_offset], 0);
    }
  
    // Execute the commands
    for (u32 command_n = 0; command_n < command_count; command_n++)
    {
      wait_for_vsync_field(commands[vsync_pos[vsync_n]].field);
      
      command_t* curr_command = &commands[command_n];

      switch (curr_command->tag)
      {
        case GS_DUMP_TRANSFER:
        {
          gs_transfer(&command_data[curr_command->command_data_offset], curr_command->size);

          break;
        }
        case GS_DUMP_VSYNC:
        {
          
          PRINTF("%05d: %d: Vsync: Expected field: %d, Actual field %lld\n",
            command_n, vsync_n, curr_command->field, (*GS_REG_CSR >> 13) & 1);

          graph_wait_vsync();
          
          vsync_n++;

          // Set the registers for the next Vsync
          if (vsync_n < vsync_count)
            gs_set_priv_regs((priv_regs_out_t*)&command_data[commands[vsync_reg_pos[vsync_n]].command_data_offset], 0);

          break;
        }
        case GS_DUMP_FIFO:
        {
          PRINTF("%05d: Fifo %d\n", command_n, curr_command->size);

          break;
        }
        case GS_DUMP_REGISTERS:
        {
          PRINTF("%05d: Registers\n", command_n);

          // Don't set the registers here because they are set in the VSYNC command
          // gs_set_priv_regs((priv_regs_out_t*)&command_data[curr_command->command_data_offset], 0);

          break;
        }
      }
    }
  }
}

int main(int argc, char* argv[])
{
  exec_gs_dump(10000);
  return 0;
}