typedef unsigned char u8;

u8 mem[1232] __attribute__((aligned(16))) = {0,};

typedef struct {
  int tag;
  union
  {
    int path; // For tag == GS_DUMP_TRANSFER
    int field; // For tag == GS_DUMP_VSYNC
  };
  int size;
  int command_data_offset; // Offset in command_data. Must always be quad word aligned.
} command_t;

command_t x[2] __attribute__((aligned(16))) = {{ 1, 2, 3, 5 }, { 1, 2, 3, 5 }, };

command_t commands[4846] __attribute__((aligned(16))) = {
  { 0, {3}, 368, 0 },
  { 0, {3}, 7568, 368 },
  { 0, {3}, 64, 7936 },
  { 0, {3}, 64, 8000 },
  { 0, {3}, 64, 8064 },
};

int main()
{
  
  return 0;
}