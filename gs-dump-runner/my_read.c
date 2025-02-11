#include <string.h>
#include "my_read.h"

const u8* my_read8(const u8* data, u8* value)
{
  *value = *data;
  return data + 1;
}

const u8* my_read16(const u8* data, u16* value)
{
  data = my_read8(data, (u8*)value);
  data = my_read8(data, (u8*)value + 1);
  return data;
}

const u8* my_read32(const u8* data, u32* value)
{
  data = my_read16(data, (u16*)value);
  data = my_read16(data, (u16*)value + 1);
  return data;
}

const u8* my_read64(const u8* data, u64* value)
{
  data = my_read32(data, (u32*)value);
  data = my_read32(data, (u32*)value + 1);
  return data;
}

const u8* my_read128(const u8* data, u128* value)
{
  data = my_read64(data, (u64*)value);
  data = my_read64(data, (u64*)value + 1);
  return data;
}

const u8* my_readqword(const u8* data, qword_t* value)
{
  data = my_read64(data, &value->dw[0]);
  data = my_read64(data, &value->dw[1]);
  return data;
}

const u8* my_readf32(const u8* data, float* value)
{
  return my_read32(data, (u32*)value);
}

const u8* my_readf64(const u8* data, double* value)
{
  return my_read64(data, (u64*)value);
}

const u8* my_read(const u8* data, u8 *value, u32 size)
{
  memcpy(value, data, size);
  return data + size;
}

const u8* my_read_ptr(const u8* data, const void** value, u32 size)
{
  *value = data;
  return data + size;
}

