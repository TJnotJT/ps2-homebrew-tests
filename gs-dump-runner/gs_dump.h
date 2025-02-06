#include <tamtypes.h>

#ifndef __GS_DUMP_H__
#define __GS_DUMP_H__

#define REGISTERS_SIZE 8192
#define COMMAND_BUFFER_SIZE 8192

typedef struct
{
  u32 old;
  u32 state_version;
  u32 state_size;
  u32 serial_offset;
  u32 serial_size;
  u32 crc;
  u32 screenshot_width;
  u32 screenshot_height;
  u32 screenshot_offset;
  u32 screenshot_size;
} gs_dump_header_t;

enum
{
  GS_DUMP_TRANSFER = 0,
  GS_DUMP_VSYNC = 1,
  GS_DUMP_FIFO = 2,
  GS_DUMP_REGISTERS = 3,
  GS_DUMP_FREEZE = 4,
  GS_DUMP_UNKNOWN = 5
};

typedef struct
{
  u8 tag;
  union
  {
    u8 path; // For GS_DUMP_TRANSFER
    u8 field; // For GS_DUMP_VSYNC
  };
  u32 size;
  const u8* data;
} gs_dump_command_t;

typedef struct
{
	gs_dump_header_t header;
  u8* state;
  u8* registers;
  gs_dump_command_t commands[COMMAND_BUFFER_SIZE];
  u32 command_count;
} gs_dump_t;

#endif