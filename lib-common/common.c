
#include <assert.h>
#include <tamtypes.h>

u32 _yx_to_block_32[4][8] = {
  {  0,  1,  4,  5, 16, 17, 20, 21},
	{  2,  3,  6,  7, 18, 19, 22, 23},
	{  8,  9, 12, 13, 24, 25, 28, 29},
	{ 10, 11, 14, 15, 26, 27, 30, 31}
};

u32 _block_to_yx_32[32][2];

u32 _yx_to_word_32[8][8] =
{
	{  0,  1,  4,  5,  8,  9, 12, 13 },
	{  2,  3,  6,  7, 10, 11, 14, 15 },
	{ 16, 17, 20, 21, 24, 25, 28, 29 },
	{ 18, 19, 22, 23, 26, 27, 30, 31 },
	{ 32, 33, 36, 37, 40, 41, 44, 45 },
	{ 34, 35, 38, 39, 42, 43, 46, 47 },
	{ 48, 49, 52, 53, 56, 57, 60, 61 },
	{ 50, 51, 54, 55, 58, 59, 62, 63 },
};

u32 _word_to_yx_32[64][2];

void init_tables()
{
  for (int y = 0; y < 4; y++)
  {
    for (int x = 0; x < 8; x++)
    {
      _block_to_yx_32[_yx_to_block_32[y][x]][0] = y;
      _block_to_yx_32[_yx_to_block_32[y][x]][1] = x;
    }
  }
  for (int y = 0; y < 8; y++)
  {
    for (int x = 0; x < 8; x++)
    {
      _word_to_yx_32[_yx_to_word_32[y][x]][0] = y;
      _word_to_yx_32[_yx_to_word_32[y][x]][1] = x;
    }
  }
}

int xy_to_block_32(int x, int y)
{
  assert(0 <= x < 64 && 0 <= y < 32);
  return _block_to_yx_32[y / 8][x / 8];
}

int xy_to_word_32(int x, int y)
{
  assert(0 <= x < 64 && 0 <= y < 32);
  return _word_to_yx_32[y / 4][x / 8];
}