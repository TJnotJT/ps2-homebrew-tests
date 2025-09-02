#include "common.h"
#include "swizzle.h"

u8 dump[0x10000000]; // Raw GS dump data
u32 dump_size;
u32 dump_curr = 0; // Points to next byte to be read

u32 crc; // CRC of the GS dump

// GS Dump header
u32 header_size;
u32 header_old;
u32 header_state_version;
u32 header_state_size;
u32 header_serial_offset;
u32 header_serial_size;
u32 header_crc;
u32 header_screenshot_width;
u32 header_screenshot_height;
u32 header_screenshot_offset;
u32 header_screenshot_size;

priv_regs_out_t priv_regs_init; // Initial GS privileged registers

general_regs_t general_regs_init __attribute__((aligned(16))); // Initial GS general registers
u32 general_regs_init_size;

u8 memory_init[0x400000]; // Initial memory for the GS

command_t commands[0x100000]; // Processed GS dump commands
u32 command_count = 0;

// The VSYNC and REGISTER commands should be paired one-to-one
// The REGISTER command should come right before the VSYNC command
u32 vsync_pos[0x1000]; // Positions of GS_DUMP_VSYNC commands in commands
u32 vsync_reg_pos[0x1000]; // Positions of GS_DUMP_REGISTERS commands in commands
u32 vsync_count = 0;

u8 command_data[0x10000000]; // Processed GS dump data for GS_DUMP_REGISTERS and GS_DUMP_TRANSFER
u32 command_data_curr = 0; // Points to next byte to be read

FILE* debug_file = 0;

void debug_printf(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(debug_file, format, args);
  va_end(args);
}

u8 read_u8()
{
  u8 value = dump[dump_curr];
  dump_curr++;
  return value;
}

u32 read_u32()
{
  u32 value = *(u32*)&dump[dump_curr];
  dump_curr += 4;
  return value;
}

u64 read_u64()
{
  u64 value = *(u64*)&dump[dump_curr];
  dump_curr += 8;
  return value;
}

void read(u8* dest, u32 size)
{
  memcpy(dest, &dump[dump_curr], size);
  dump_curr += size;
}

void align(u32* ptr, u64 size)
{
  *ptr = (((u32)*ptr + size - 1) / size) * size;
}

void read_dump_header()
{
  crc = read_u32();
  if (crc == 0xFFFFFFFF)
  {
    header_old = 0;
    header_size = read_u32();
    header_state_version = read_u32();
    header_state_size = read_u32();
    header_serial_offset = read_u32();
    header_serial_size = read_u32();
    header_crc = read_u32();
    header_screenshot_width = read_u32();
    header_screenshot_height = read_u32();
    header_screenshot_offset = read_u32();
    header_screenshot_size = read_u32();
    
    debug_printf("New header\n");
    debug_printf("  header size: %d\n", header_size);
    debug_printf("  state_version: %d\n", header_state_version);
    debug_printf("  state_size: %d\n", header_state_size);
    debug_printf("  serial_offset: %d\n", header_serial_offset);
    debug_printf("  serial_size: %d\n", header_serial_size);
    debug_printf("  crc: %x\n", header_crc);
    debug_printf("  screenshot_width: %d\n", header_screenshot_width);
    debug_printf("  screenshot_height: %d\n", header_screenshot_height);
    debug_printf("  screenshot_offset: %d\n", header_screenshot_offset);
    debug_printf("  screenshot_size: %d\n", header_screenshot_size);
  }
  else
  {
    header_old = 1;
    header_state_size = read_u32();
    header_state_version = read_u32();
    header_crc = crc;
    
    debug_printf("Old header\n");
    debug_printf("  state_size: %d\n", header_state_size);
    debug_printf("  state_version: %d\n", header_state_version);
  }
}

void read_general_regs()
{
  general_regs_init.PRIM = read_u64(); // PRIM

  if (header_state_version <= 6)
    read_u64(); // PRMODE

  general_regs_init.PRMODECONT = read_u64(); // PRMODECONT
  general_regs_init.TEXCLUT = read_u64(); // TEXCLUT
  general_regs_init.SCANMSK = read_u64(); // SCANMSK
  general_regs_init.TEXA = read_u64(); // TEXA
  general_regs_init.FOGCOL = read_u64(); // FOGCOL
  general_regs_init.DIMX = read_u64(); // DIMX
  general_regs_init.DTHE = read_u64(); // DTHE
  general_regs_init.COLCLAMP = read_u64(); // COLCLAMP
  general_regs_init.PABE = read_u64(); // PABE
  general_regs_init.BITBLTBUF = read_u64(); // BITBLTBUF
  general_regs_init.TRXDIR = read_u64(); // TRXDIR
  general_regs_init.TRXPOS = read_u64(); // TRXPOS
  general_regs_init.TRXREG_1 = read_u64(); // TRXREG
  general_regs_init.TRXREG_2 = read_u64(); // TRXREG

  // Context 1
  general_regs_init.XYOFFSET_1 = read_u64(); // XYOFFSET_1
  general_regs_init.TEX0_1 = read_u64(); // TEX0_1
  general_regs_init.TEX1_1 = read_u64(); // TEX1_1
  if (header_state_version <= 6)
    read_u64(); // TEX2
  general_regs_init.CLAMP_1 = read_u64(); // CLAMP_1
  general_regs_init.MIPTBP1_1 = read_u64(); // MIPTBP1_1
  general_regs_init.MIPTBP2_1 = read_u64(); // MIPTBP2_1
  general_regs_init.SCISSOR_1 = read_u64(); // SCISSOR_1
  general_regs_init.ALPHA_1 = read_u64(); // ALPHA_1
  general_regs_init.TEST_1 = read_u64(); // TEST_1
  general_regs_init.FBA_1 = read_u64(); // FBA_1
  general_regs_init.FRAME_1 = read_u64(); // FRAME_1
  general_regs_init.ZBUF_1 = read_u64(); // ZBUF_1
  
  if (header_state_version <= 4)
    for (int i = 0; i < 7; i++)
      read_u32(); // skip ???
  
  // Context 2
  general_regs_init.XYOFFSET_2 = read_u64(); // XYOFFSET_2
  general_regs_init.TEX0_2 = read_u64(); // TEX0_2
  general_regs_init.TEX1_2 = read_u64(); // TEX1_2
  if (header_state_version <= 6)
    read_u64(); // TEX2
  general_regs_init.CLAMP_2 = read_u64(); // CLAMP_2
  general_regs_init.MIPTBP1_2 = read_u64(); // MIPTBP1_2
  general_regs_init.MIPTBP2_2 = read_u64(); // MIPTBP2_2
  general_regs_init.SCISSOR_2 = read_u64(); // SCISSOR_2
  general_regs_init.ALPHA_2 = read_u64(); // ALPHA_2
  general_regs_init.TEST_2 = read_u64(); // TEST_2
  general_regs_init.FBA_2 = read_u64(); // FBA_2
  general_regs_init.FRAME_2 = read_u64(); // FRAME_2
  general_regs_init.ZBUF_2 = read_u64(); // ZBUF_2
  
  if (header_state_version <= 4)
    for (int i = 0; i < 7; i++)
      read_u32(); // skip ???

  general_regs_init.RGBAQ = read_u64(); // RGBAQ
  general_regs_init.ST = read_u64(); // ST
  general_regs_init.UV = read_u32(); // UV
  general_regs_init.FOG = read_u32(); // FOG
  general_regs_init.XYZ2 = read_u64(); // XYZ2

  read_u64(); // obsolete apparently
  
  // Unsure how to use these
  read_u32(); // m_tr.x
  read_u32(); // m_tr.y
  
  // There is more data after, but it is not used
}

void read_memory()
{
  u8 temp_memory[GS_MEMORY_SIZE] __attribute__((aligned(16)));
  read(temp_memory, GS_MEMORY_SIZE);
  deswizzleImage(memory_init, temp_memory, 1024 / 64, 1024 / 32);
}

void read_priv_regs(priv_regs_out_t* priv_regs)
{
  priv_reg_in_t* priv_regs_in = (priv_reg_in_t*)&dump[dump_curr];
  priv_regs->PMODE = priv_regs_in->PMODE;
  priv_regs->SMODE1 = priv_regs_in->SMODE1;
  priv_regs->SMODE2 = priv_regs_in->SMODE2;
  priv_regs->SRFSH = priv_regs_in->SRFSH;
  priv_regs->SYNCH1 = priv_regs_in->SYNCH1;
  priv_regs->SYNCH2 = priv_regs_in->SYNCH2;
  priv_regs->SYNCV = priv_regs_in->SYNCV;
  priv_regs->DISP[0].DISPFB = priv_regs_in->DISP[0].DISPFB;
  priv_regs->DISP[0].DISPLAY = priv_regs_in->DISP[0].DISPLAY;
  priv_regs->DISP[1].DISPFB = priv_regs_in->DISP[1].DISPFB;
  priv_regs->DISP[1].DISPLAY = priv_regs_in->DISP[1].DISPLAY;
  priv_regs->EXTBUF = priv_regs_in->EXTBUF;
  priv_regs->EXTDATA = priv_regs_in->EXTDATA;
  priv_regs->EXTWRITE = priv_regs_in->EXTWRITE;
  priv_regs->BGCOLOR = priv_regs_in->BGCOLOR;
  priv_regs->CSR = priv_regs_in->CSR;
  priv_regs->IMR = priv_regs_in->IMR;
  priv_regs->BUSDIR = priv_regs_in->BUSDIR;
  priv_regs->SIGLBLID = priv_regs_in->SIGLBLID;

  dump_curr += sizeof(priv_reg_in_t);
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s <in dump file> <out data file> <out variable file>\n", argv[0]);
    return 1;
  }
  
  debug_file = stderr;
  debug_file = fopen("preprocess_dump_debug.txt", "w");
  FILE* file = fopen(argv[1], "rb");

  if (!file)
  {
    fprintf(stderr, "Failed to open file\n");
    fclose(debug_file);
    return 1;
  }

  debug_printf("Opened file\n");

  // Read all data into the dump buffer
  fseek(file, 0, SEEK_END);
  dump_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  fread(dump, 1, dump_size, file);
  fclose(file);

  printf("Reading header\n");
  fprintf(debug_file, "Reading header\n");
  read_dump_header();
  debug_printf("%08x: Header\n", dump_curr);

  // Skip over the header data
  dump_curr += header_serial_size;
  dump_curr += header_screenshot_size;

  if (!header_old)
    dump_curr += sizeof(header_state_version); // Skip state version

  const u32 state_size = header_state_size - 4;
  u64 dump_curr_0 = dump_curr; // Save this for later

  read_general_regs();
  debug_printf("%08x: Registers\n", dump_curr);

  read_memory();
  debug_printf("%08x: Memory\n", dump_curr);

  // read_general_regs() and read_memory() read less than state_size
  dump_curr = dump_curr_0 + state_size;
  
  debug_printf("%08x: State\n", dump_curr);
  
  read_priv_regs(&priv_regs_init);

  debug_printf("%08x: Registers\n", dump_curr);

  u32 prev_tag = GS_DUMP_UNKNOWN;

  while (dump_curr < dump_size)
  {
    command_t* curr_command = &commands[command_count];
    curr_command->tag = read_u8();
    debug_printf("%08x: Tag %d\n", dump_curr, curr_command->tag);

    switch (curr_command->tag)
    {
      case GS_DUMP_TRANSFER:
      {
        curr_command->path = read_u8();
        curr_command->size = read_u32();
        align(&command_data_curr, 16);
        curr_command->command_data_offset = command_data_curr;
        read(&command_data[command_data_curr], curr_command->size);
        command_data_curr += curr_command->size;

        debug_printf("%08x: Transfer %d %d\n", dump_curr, curr_command->path, curr_command->size);
        break;
      }
      case GS_DUMP_VSYNC:
      {
        assert(prev_tag == GS_DUMP_REGISTERS);
        vsync_pos[vsync_count++] = command_count;
        curr_command->field = read_u8();

        debug_printf("%08x: Vsync %d\n", dump_curr, curr_command->field);
        break;
      }
      case GS_DUMP_FIFO:
      {
        curr_command->size = read_u32();

        debug_printf("%08x: Fifo %d\n", dump_curr, curr_command->size);
        break;
      }
      case GS_DUMP_REGISTERS:
      {
        vsync_reg_pos[vsync_count] = command_count;

        align(&command_data_curr, 16);
        curr_command->command_data_offset = command_data_curr;
        read_priv_regs((priv_regs_out_t*)&command_data[command_data_curr]);
        curr_command->size = sizeof(priv_regs_out_t);
        command_data_curr += curr_command->size;

        debug_printf("%08x: Registers\n", dump_curr);
        break;
      }
      case GS_DUMP_FREEZE:
      {
        fprintf(stderr, "Freeze not supported\n");
        fclose(debug_file);
        return 1;
      }
      case GS_DUMP_UNKNOWN:
      {
        fprintf(stderr, "Unknown tag\n");
        fclose(debug_file);
        return 1;
      }
    }

    prev_tag = curr_command->tag;
    command_count++;
  }

  fprintf(stdout, "Processed %d commands\n", command_count);
  debug_printf("Processed %d commands\n", command_count);

  // Write the data to the output file
  FILE* data_file = fopen(argv[2], "wb");

  fprintf(data_file, "#include \"common.h\"\n\n");

  // Write the initial privileged registers
  fprintf(data_file, "priv_regs_out_t priv_regs_init __attribute__((aligned(16))) = {\n");
  fprintf(data_file, "  .PMODE = 0x%016llx,\n", priv_regs_init.PMODE);
  fprintf(data_file, "  .SMODE1 = 0x%016llx,\n", priv_regs_init.SMODE1);
  fprintf(data_file, "  .SMODE2 = 0x%016llx,\n", priv_regs_init.SMODE2);
  fprintf(data_file, "  .SRFSH = 0x%016llx,\n", priv_regs_init.SRFSH);
  fprintf(data_file, "  .SYNCH1 = 0x%016llx,\n", priv_regs_init.SYNCH1);
  fprintf(data_file, "  .SYNCH2 = 0x%016llx,\n", priv_regs_init.SYNCH2);
  fprintf(data_file, "  .SYNCV = 0x%016llx,\n", priv_regs_init.SYNCV);
  fprintf(data_file, "  .DISP = {\n");
  fprintf(data_file, "    { .DISPFB = 0x%016llx, .DISPLAY = 0x%016llx },\n", priv_regs_init.DISP[0].DISPFB, priv_regs_init.DISP[0].DISPLAY);
  fprintf(data_file, "    { .DISPFB = 0x%016llx, .DISPLAY = 0x%016llx },\n", priv_regs_init.DISP[1].DISPFB, priv_regs_init.DISP[1].DISPLAY);
  fprintf(data_file, "  },\n");
  fprintf(data_file, "  .EXTBUF = 0x%016llx,\n", priv_regs_init.EXTBUF);
  fprintf(data_file, "  .EXTDATA = 0x%016llx,\n", priv_regs_init.EXTDATA);
  fprintf(data_file, "  .EXTWRITE = 0x%016llx,\n", priv_regs_init.EXTWRITE);
  fprintf(data_file, "  .BGCOLOR = 0x%016llx,\n", priv_regs_init.BGCOLOR);
  fprintf(data_file, "  .CSR = 0x%016llx,\n", priv_regs_init.CSR);
  fprintf(data_file, "  .IMR = 0x%016llx,\n", priv_regs_init.IMR);
  fprintf(data_file, "  .BUSDIR = 0x%016llx,\n", priv_regs_init.BUSDIR);
  fprintf(data_file, "  .SIGLBLID = 0x%016llx,\n", priv_regs_init.SIGLBLID);
  fprintf(data_file, "};\n\n");

  // Write the initial context registers
  fprintf(data_file, "u32 general_regs_init_size = %d;\n\n", general_regs_init_size);
  fprintf(data_file, "general_regs_t general_regs_init  = {\n");
  fprintf(data_file, "  .PRIM = 0x%016llx,\n", general_regs_init.PRIM);
  fprintf(data_file, "  .PRMODECONT = 0x%016llx,\n", general_regs_init.PRMODECONT);
  fprintf(data_file, "  .TEXCLUT = 0x%016llx,\n", general_regs_init.TEXCLUT);
  fprintf(data_file, "  .SCANMSK = 0x%016llx,\n", general_regs_init.SCANMSK);
  fprintf(data_file, "  .TEXA = 0x%016llx,\n", general_regs_init.TEXA);
  fprintf(data_file, "  .FOGCOL = 0x%016llx,\n", general_regs_init.FOGCOL);
  fprintf(data_file, "  .DIMX = 0x%016llx,\n", general_regs_init.DIMX);
  fprintf(data_file, "  .DTHE = 0x%016llx,\n", general_regs_init.DTHE);
  fprintf(data_file, "  .COLCLAMP = 0x%016llx,\n", general_regs_init.COLCLAMP);
  fprintf(data_file, "  .PABE = 0x%016llx,\n", general_regs_init.PABE);
  fprintf(data_file, "  .BITBLTBUF = 0x%016llx,\n", general_regs_init.BITBLTBUF);
  fprintf(data_file, "  .TRXDIR = 0x%016llx,\n", general_regs_init.TRXDIR);
  fprintf(data_file, "  .TRXPOS = 0x%016llx,\n", general_regs_init.TRXPOS);
  fprintf(data_file, "  .TRXREG_1 = 0x%016llx,\n", general_regs_init.TRXREG_1);
  fprintf(data_file, "  .TRXREG_2 = 0x%016llx,\n", general_regs_init.TRXREG_2);
  fprintf(data_file, "  .XYOFFSET_1 = 0x%016llx,\n", general_regs_init.XYOFFSET_1);
  fprintf(data_file, "  .TEX0_1 = 0x%016llx,\n", general_regs_init.TEX0_1);
  fprintf(data_file, "  .TEX1_1 = 0x%016llx,\n", general_regs_init.TEX1_1);
  fprintf(data_file, "  .CLAMP_1 = 0x%016llx,\n", general_regs_init.CLAMP_1);
  fprintf(data_file, "  .MIPTBP1_1 = 0x%016llx,\n", general_regs_init.MIPTBP1_1);
  fprintf(data_file, "  .MIPTBP2_1 = 0x%016llx,\n", general_regs_init.MIPTBP2_1);
  fprintf(data_file, "  .SCISSOR_1 = 0x%016llx,\n", general_regs_init.SCISSOR_1);
  fprintf(data_file, "  .ALPHA_1 = 0x%016llx,\n", general_regs_init.ALPHA_1);
  fprintf(data_file, "  .TEST_1 = 0x%016llx,\n", general_regs_init.TEST_1);
  fprintf(data_file, "  .FBA_1 = 0x%016llx,\n", general_regs_init.FBA_1);
  fprintf(data_file, "  .FRAME_1 = 0x%016llx,\n", general_regs_init.FRAME_1);
  fprintf(data_file, "  .ZBUF_1 = 0x%016llx,\n", general_regs_init.ZBUF_1);
  fprintf(data_file, "  .XYOFFSET_2 = 0x%016llx,\n", general_regs_init.XYOFFSET_2);
  fprintf(data_file, "  .TEX0_2 = 0x%016llx,\n", general_regs_init.TEX0_2);
  fprintf(data_file, "  .TEX1_2 = 0x%016llx,\n", general_regs_init.TEX1_2);
  fprintf(data_file, "  .CLAMP_2 = 0x%016llx,\n", general_regs_init.CLAMP_2);
  fprintf(data_file, "  .MIPTBP1_2 = 0x%016llx,\n", general_regs_init.MIPTBP1_2);
  fprintf(data_file, "  .MIPTBP2_2 = 0x%016llx,\n", general_regs_init.MIPTBP2_2);
  fprintf(data_file, "  .SCISSOR_2 = 0x%016llx,\n", general_regs_init.SCISSOR_2);
  fprintf(data_file, "  .ALPHA_2 = 0x%016llx,\n", general_regs_init.ALPHA_2);
  fprintf(data_file, "  .TEST_2 = 0x%016llx,\n", general_regs_init.TEST_2);
  fprintf(data_file, "  .FBA_2 = 0x%016llx,\n", general_regs_init.FBA_2);
  fprintf(data_file, "  .FRAME_2 = 0x%016llx,\n", general_regs_init.FRAME_2);
  fprintf(data_file, "  .ZBUF_2 = 0x%016llx,\n", general_regs_init.ZBUF_2);
  fprintf(data_file, "  .RGBAQ = 0x%016llx,\n", general_regs_init.RGBAQ);
  fprintf(data_file, "  .ST = 0x%016llx,\n", general_regs_init.ST);
  fprintf(data_file, "  .UV = 0x%08llx,\n", general_regs_init.UV);
  fprintf(data_file, "  .FOG = 0x%08llx,\n", general_regs_init.FOG);
  fprintf(data_file, "  .XYZ2 = 0x%016llx,\n", general_regs_init.XYZ2);
  fprintf(data_file, "};\n\n");

  // Write the memory
  fprintf(data_file, " u8 memory_init[0x%x] __attribute__((aligned(16))) = {\n", GS_MEMORY_SIZE);
  for (u32 i = 0; i < GS_MEMORY_SIZE; i++)
  {
    if (i % 16 == 0)
      fprintf(data_file, "  ");
    fprintf(data_file, "0x%02x, ", memory_init[i]);
    if (i % 16 == 15)
      fprintf(data_file, "\n");
  }
  fprintf(data_file, "};\n\n");

  // Write the command structure
  fprintf(data_file, "u32 command_count = %d;\n\n", command_count);
  fprintf(data_file, "command_t commands[%d] __attribute__((aligned(16))) = {\n", command_count);
  for (u32 i = 0; i < command_count; i++)
  {
    command_t* curr_command = &commands[i];
    fprintf(data_file, "  { %d, {%d}, %d, %d },\n", curr_command->tag, curr_command->path, curr_command->size, curr_command->command_data_offset);
  }
  fprintf(data_file, "};\n\n");

  // Write the VSYNC and REGISTER positions
  fprintf(data_file, "u32 vsync_count = %d;\n\n", vsync_count);

  fprintf(data_file, "u32 vsync_pos[%d] = {\n", vsync_count);
  for (u32 i = 0; i < vsync_count; i++)
    fprintf(data_file, "  %d,\n", vsync_pos[i]);
  fprintf(data_file, "};\n\n");

  fprintf(data_file, "u32 vsync_reg_pos[%d] = {\n", vsync_count);
  for (u32 i = 0; i < vsync_count; i++)
    fprintf(data_file, "  %d,\n", vsync_reg_pos[i]);
  fprintf(data_file, "};\n\n");

  // Write the command data
  fprintf(data_file, "u8 command_data[%d] __attribute__((aligned(16))) = {\n", command_data_curr);
  for (u32 i = 0; i < command_data_curr; i++)
  {
    if (i % 16 == 0)
      fprintf(data_file, "  ");
    fprintf(data_file, "0x%02x, ", command_data[i]);
    if (i % 16 == 15)
      fprintf(data_file, "\n");
  }
  fprintf(data_file, "};\n\n");

  fclose(data_file);

  printf("Wrote output data\n");

  // Write the variable file
  FILE* var_file = fopen(argv[3], "w");
  fprintf(var_file, "#include \"common.h\"\n\n");
  fprintf(var_file, "extern priv_regs_out_t priv_regs_init __attribute__((aligned(16)));\n");
  fprintf(var_file, "extern u32 general_regs_init_size;\n");
  fprintf(var_file, "extern general_regs_t general_regs_init __attribute__((aligned(16)));\n");
  fprintf(var_file, "extern u8 memory_init[0x%x] __attribute__((aligned(16)));\n", GS_MEMORY_SIZE);
  fprintf(var_file, "extern u32 command_count;\n");
  fprintf(var_file, "extern command_t commands[%d] __attribute__((aligned(16)));\n", command_count);
  fprintf(var_file, "extern u32 vsync_count;\n");
  fprintf(var_file, "extern u32 vsync_pos[%d];\n", vsync_count);
  fprintf(var_file, "extern u32 vsync_reg_pos[%d];\n", vsync_count);
  fprintf(var_file, "extern u8 command_data[%d] __attribute__((aligned(16)));\n", command_data_curr);
  fclose(var_file);

  printf("Wrote variable file\n");

  fclose(debug_file);
  return 0;
}