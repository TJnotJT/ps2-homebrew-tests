#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include <gif_tags.h>
#include <gs_psm.h>
#include <gs_privileged.h>
#include <gs_gp.h>

#include <sio.h>

#include "gs_dump.h"
#include "my_read.h"

gs_dump_t dump;

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

  data = my_read_ptr(data, (const void**)&dump.registers, REGISTERS_SIZE);
  printf("%08x: Registers\n", data - data_start);

  gs_dump_command_t* curr_command = dump.commands;
  
  u32 num_commands = 0;
  while (data < data_end)
  {
    num_commands++;
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
        break;
      }
      case GS_DUMP_VSYNC:
      {
        data = my_read8(data, &curr_command->field);

        printf("%08x: Vsync %d\n", data - data_start, curr_command->field);
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
  read_gs_dump(pcsx2_dump, pcsx2_dump + size_pcsx2_dump);
  return 0;
}