//By Monica Moniot
#ifndef INCLUDE__BASIC_H
#define INCLUDE__BASIC_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


typedef uint8_t  byte;
typedef uint32_t uint;
typedef int8_t   int8;
typedef uint8_t  uint8;
typedef int16_t  int16;
typedef uint16_t uint16;
typedef int32_t  int32;
typedef uint32_t uint32;
typedef int64_t  int64;
typedef uint64_t uint64;

#define KILOBYTE (((int64)1)<<10)
#define MEGABYTE (((int64)1)<<20)
#define GIGABYTE (((int64)1)<<30)
#define TERABYTE (((int64)1)<<40)

#define MACRO_CAT_(a, b) a ## b
#define MACRO_CAT(a, b) MACRO_CAT_(a, b)
#define UNIQUE_NAME(prefix) MACRO_CAT(prefix, __LINE__)

#define NULLOP() 0
#define ERROR() (*((byte*)(0x1337)) = 0)
#define INVALID_EXECUTION ERROR

#define cast(type, value) ((type)(value))
#define from_cstr(str) str, strlen(str)
#define ptr_add(type, ptr, n) ((type*)((byte*)(ptr) + (n)))
#define ptr_dist(ptr0, ptr1) ((int64)((byte*)(ptr1) - (byte*)(ptr0)))
#define memzero(ptr, size) memset(ptr, 0, size)

#define for_each_lt(name, size) int32 UNIQUE_NAME(name) = (size); for(int32 name = 0; name < UNIQUE_NAME(name); name += 1)
#define for_each_lt_bw(name, size) for(int32 name = (size) - 1; name >= 0; name -= 1)
#define for_each_in_range(name, r0, r1) int32 UNIQUE_NAME(name) = (r1); for(int32 name = (r0); name <= UNIQUE_NAME(name); name += 1)
#define for_each_in_range_bw(name, r0, r1) int32 UNIQUE_NAME(name) = (r0); for(int32 name = (r1); name >= UNIQUE_NAME(name); name -= 1)
#define for_ever(name) for(int32 name = 0;; name += 1)

#ifdef __cplusplus
	#define swap(v0, v1) auto UNIQUE_NAME(__t) = *(v0); *(v0) = *(v1); *(v1) = UNIQUE_NAME(__t)
	#define for_each_in(name, array, size) auto UNIQUE_NAME(name) = (array) + (size); for(auto name = (array); name != UNIQUE_NAME(name); name += 1)
	#define for_each_in_bw(name, array, size) auto UNIQUE_NAME(name) = (array) - 1; for(auto name = (array) + (size) - 1; name != UNIQUE_NAME(name); name -= 1)
#endif


#ifdef __cplusplus
}
#endif
#endif
