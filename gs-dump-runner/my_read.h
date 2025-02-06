#include <tamtypes.h>

#ifndef __READ_H__
#define __READ_H__

const u8* my_read(const u8* data, u8* value, u32 size);
const u8* my_read_ptr(const u8* data, const void** value, u32 size);
const u8* my_read8(const u8* data, u8* value);
const u8* my_read16(const u8* data, u16* value);
const u8* my_read32(const u8* data, u32* value);
const u8* my_read64(const u8* data, u64* value);
const u8* my_read128(const u8* data, u128* value);
const u8* my_readf32(const u8* data, float* value);
const u8* my_readf64(const u8* data, double* value);

#endif