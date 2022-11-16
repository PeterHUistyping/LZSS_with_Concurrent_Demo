/* 

  I/O part of Compression and Decompression
  With Comments and Documentation 
  Copyright (C) 2022 PeterHUistyping, Samuel Jie, Crazy_Thursday

  -----------------------------------------------------------------
  
  Reference:

  RFC 1951 / DEFLATE Compressed Data Format Specification

  Ziv J., Lempel A., "A Universal Algorithm for sequenceuential Data
  Compression", IEEE Transactions on Information Theory, Vol. 23,
  No. 3, pp. 337-343.

  Lempel–Ziv–Storer–Szymanski 
  https://en.wikipedia.org/wiki/Lempel–Ziv–Storer–Szymanski#cite_note-1
  https://en.wikipedia.org/wiki/LZ77_and_LZ78#LZ77

  Data Compression - Lecture 10 - Lempel-Ziv Schemes
  https://www.youtube.com/watch?v=VDXBnmr8AY0&list=PLU4IQLU9e_OpnkbCS_to64F_vw5yyg4HB&index=3

  ------------------------------------------------------------------------------------------
*/
#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
using ubyte = uint8_t;
using uint=uint32_t;
using ull = uint64_t;
#define WINDOW_SIZE_L1 8192
#define MAX_L2_Length 8191
#define MAX_L3_Length 65535
#define LEVEL1_MAX 8192*8 //65536
#define HASH_SIZE (1 << 14)  //HASH_LOG 14
#define Hash_Key_Masks (HASH_SIZE-1)
#define Extra_Window_Size 65535 //level4

// Return a 8-byte unsigned integer from the current stream
ull read_uint64(const void* ptr) { 
  const ull* ptr_to64=(const ull*)ptr;
  return ptr_to64[0]; 
}

// Return a 4-byte unsigned integer from the current stream 
uint read_uint32(const void* ptr) { 
  const uint* ptr_to32=(const uint*)ptr;
  return ptr_to32[0]; 
}

//return the same len + 1 from the start of p; if no matched return 1
uint match_cmp(const ubyte* ref, const ubyte* input, const ubyte* bound) {
  const ubyte* start = ref;
  //check whether 64 Bytes are the same or not
  if (read_uint64(ref) == read_uint64(input)) {
    ref += 8,input += 8;
  }
  //check whether 32 Bytes are the same or not
  if (read_uint32(ref) == read_uint32(input)) {
    ref += 4,input += 4;
  }
  while (input < bound){
    if (*ref++ != *input++) break;
  }
  return ref - start;
}
