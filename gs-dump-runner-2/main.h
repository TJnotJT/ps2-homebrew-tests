#ifndef _MAIN_H_
#define _MAIN_H_

#include <tamtypes.h> // u8, u32, u64

#define GS_REGISTERS_SIZE 8192
#define GS_STATE_SIZE 0x401000
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

typedef enum
{
  GS_DUMP_TRANSFER = 0,
  GS_DUMP_VSYNC = 1,
  GS_DUMP_FIFO = 2,
  GS_DUMP_REGISTERS = 3,
  GS_DUMP_FREEZE = 4,
  GS_DUMP_UNKNOWN = 5
} gs_dump_tag_t;

typedef struct
{
  u8 tag; // GS_DUMP_* enum
  union
  {
    u8 path; // For tag == GS_DUMP_TRANSFER
    u8 field; // For tag == GS_DUMP_VSYNC
  };
  u32 size;
  const u8* data;
} gs_dump_command_t;

extern u8 gs_dump_state[GS_STATE_SIZE] __attribute((aligned(16)));

typedef struct
{
	union
	{
		struct
		{
			u64 PMODE;
			u64 _pad1;
			u64 SMODE1;
			u64 _pad2;
			u64 SMODE2;
			u64 _pad3;
			u64 SRFSH;
			u64 _pad4;
			u64 SYNCH1;
			u64 _pad5;
			u64 SYNCH2;
			u64 _pad6;
			u64 SYNCV;
			u64 _pad7;
			struct
			{
				u64 DISPFB;
				u64 _pad1;
				u64 DISPLAY;
				u64 _pad2;
			} DISP[2];
			u64 EXTBUF;
			u64 _pad8;
			u64 EXTDATA;
			u64 _pad9;
			u64 EXTWRITE;
			u64 _pad10;
			u64 BGCOLOR;
			u64 _pad11;
		};

		u8 _pad12[0x1000];
	};

	union
	{
		struct
		{
			u64 CSR;
			u64 _pad13;
			u64 IMR;
			u64 _pad14;
			u64 _unk1[4];
			u64 BUSDIR;
			u64 _pad15;
			u64 _unk2[6];
			u64 SIGLBLID;
			u64 _pad16;
		};

		u8 _pad17[0x1000];
	};
} gs_registers_t;

#endif