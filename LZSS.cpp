/* 
  Originally CITED FROM
  FastLZ - Byte-aligned LZ77 compression library
  Copyright (C) 2005-2020 Ariya Hidayat <ariya.hidayat@gmail.com>
  ---------------------------------------------------------------
  With Comments and Documentation 
  Deflate part
  Copyright (C) PeterHUistyping
  ---------------------------------------------------------------
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include <assert.h>

// C++ program to find out execution time of
// of functions
#include <algorithm>
#include <chrono>
#include <iostream>
#include<vector>
// threads
#include<thread>
using namespace std;
#define FASTLZ_VERSION 0x000500

#define FASTLZ_VERSION_MAJOR 0
#define FASTLZ_VERSION_MINOR 5
#define FASTLZ_VERSION_REVISION 0

#define FASTLZ_VERSION_STRING "0.5.0"

 const int num_threads=10;
/*
 * Always check for bound when decompressing.
 * Generally it is best to leave it defined.
 */
#define FASTLZ_SAFE
#if defined(FASTLZ_USE_SAFE_DECOMPRESSOR) && (FASTLZ_USE_SAFE_DECOMPRESSOR == 0)
#undef FASTLZ_SAFE
#endif

/*
 * Give hints to the compiler for branch prediction optimization.
 */
#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 2))
#define FASTLZ_LIKELY(c) (__builtin_expect(!!(c), 1))
#define FASTLZ_UNLIKELY(c) (__builtin_expect(!!(c), 0))
#else
#define FASTLZ_LIKELY(c) (c)
#define FASTLZ_UNLIKELY(c) (c)
#endif

/*
 * Specialize custom 64-bit implementation for speed improvements.
 */
#if defined(__x86_64__) || defined(_M_X64)
#define FLZ_ARCH64
#endif

#if defined(FASTLZ_SAFE)
#define FASTLZ_BOUND_CHECK(cond) \
  if (FASTLZ_UNLIKELY(!(cond))) return 0;
#else
#define FASTLZ_BOUND_CHECK(cond) \
  do {                           \
  } while (0)
#endif

#if defined(FASTLZ_USE_MEMMOVE) && (FASTLZ_USE_MEMMOVE == 0)

static void LZSS_memmove(uint8_t* dest, const uint8_t* src, uint32_t count) {
  do {
    *dest++ = *src++;
  } while (--count);
}

static void LZSS_memcpy(uint8_t* dest, const uint8_t* src, uint32_t count) {
  return LZSS_memmove(dest, src, count);
}

#else

#include <string.h>

static void LZSS_memmove(uint8_t* dest, const uint8_t* src, uint32_t count) {
  if ((count > 4) && (dest >= src + count)) {
    memmove(dest, src, count);
  } else {
    switch (count) {
      default:
        do {
          *dest++ = *src++;
        } while (--count);
        break;
      case 3:
        *dest++ = *src++;
      case 2:
        *dest++ = *src++;
      case 1:
        *dest++ = *src++;
      case 0:
        break;
    }
  }
}
//memcpy(dest, src, count);
static void LZSS_memcpy(uint8_t* dest, const uint8_t* src, uint32_t count) { memcpy(dest, src, count); }

#endif

#if defined(FLZ_ARCH64)

static uint32_t lzss_readu32(const void* ptr) { return *(const uint32_t*)ptr; }

static uint64_t lzss_readu64(const void* ptr) { return *(const uint64_t*)ptr; }

static uint32_t lzss_cmp(const uint8_t* p, const uint8_t* q, const uint8_t* r) {
  const uint8_t* start = p;

  if (lzss_readu64(p) == lzss_readu64(q)) {
    p += 8;
    q += 8;
  }
  if (lzss_readu32(p) == lzss_readu32(q)) {
    p += 4;
    q += 4;
  }
  while (q < r)
    if (*p++ != *q++) break;
  return p - start;
}

static void lzss_copy64(uint8_t* dest, const uint8_t* src, uint32_t count) {
  const uint64_t* p = (const uint64_t*)src;
  uint64_t* q = (uint64_t*)dest;
  if (count < 16) {
    if (count >= 8) {
      *q++ = *p++;
    }
    *q++ = *p++;
  } else {
    *q++ = *p++;
    *q++ = *p++;
    *q++ = *p++;
    *q++ = *p++;
  }
}

static void lzss_copy256(void* dest, const void* src) {
  const uint64_t* p = (const uint64_t*)src;
  uint64_t* q = (uint64_t*)dest;
  *q++ = *p++;
  *q++ = *p++;
  *q++ = *p++;
  *q++ = *p++;
}

#endif /* FLZ_ARCH64 */

#if !defined(FLZ_ARCH64)
// read in 4B / 32 bits
static uint32_t lzss_readu32(const void* ptr) {
  const uint8_t* p = (const uint8_t*)ptr;
  return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}
//return the same len + 1 from the start of p; if no matched return 1
static uint32_t lzss_cmp(const uint8_t* p, const uint8_t* q, const uint8_t* r) {
  const uint8_t* start = p;
  while (q < r)
    if (*p++ != *q++) break;
  return p - start;
}

static void lzss_copy64(uint8_t* dest, const uint8_t* src, uint32_t count) {
  const uint8_t* p = (const uint8_t*)src;
  uint8_t* q = (uint8_t*)dest;
  int c;
  for (c = 0; c < count * 8; ++c) {
    *q++ = *p++;
  }
}

static void lzss_copy256(void* dest, const void* src) {
  const uint8_t* p = (const uint8_t*)src;
  uint8_t* q = (uint8_t*)dest;
  int c;
  for (c = 0; c < 32; ++c) {
    *q++ = *p++;
  }
}

#endif /* !FLZ_ARCH64 */

#define MAX_COPY 32
#define MAX_LEN 264 /* 256 + 8 */
#define MAX_L1_DISTANCE 8192 //8KB  2^13
#define MAX_L2_DISTANCE 8191
#define MAX_FARDISTANCE (65535 + MAX_L2_DISTANCE - 1)

#define HASH_LOG 14
#define HASH_SIZE (1 << HASH_LOG)  //HASH_LOG 14
#define HASH_MASK (HASH_SIZE - 1)

//hashing the least Hash_Log bits
static uint16_t lzss_hash(uint32_t v) {
  uint32_t h = (v * 2654435769LL) >> (32 - HASH_LOG); 
  return h & HASH_MASK;
}
/**
 * @brief *dest++ = MAX_COPY - 1;
 * 
 * @param runs numbers of bytes to be written
 * @param src  start point  
 * @param dest destionation
 * @return uint8_t* 
 */
static uint8_t* lzss_literals(uint32_t runs, const uint8_t* src, uint8_t* dest) {
  while (runs >= MAX_COPY) {
    *dest++ = MAX_COPY - 1;
    lzss_copy256(dest, src);
    src += MAX_COPY;
    dest += MAX_COPY;
    runs -= MAX_COPY;
  }
  if (runs > 0) {
    *dest++ = runs - 1;  //literal nums
    lzss_copy64(dest, src, runs);
    dest += runs;
  }
  return dest;
}

/* special case of memcpy: at most 32 bytes ||||LZSS_memcpy(dest, src, count);*/
static void lzss_smallcopy(uint8_t* dest, const uint8_t* src, uint32_t count) {
#if defined(FLZ_ARCH64)
  if (count >= 8) {
    const uint64_t* p = (const uint64_t*)src;
    uint64_t* q = (uint64_t*)dest;
    while (count > 8) {
      *q++ = *p++;
      count -= 8;
      dest += 8;
      src += 8;
    }
  }
#endif
  LZSS_memcpy(dest, src, count);
}
/**
 * @brief copy src -> dest
 * LZSS_memcpy(dest, src, count);  copy the second arg -> first arg
 * 
 * @param runs copy <=Max_COPY
 * @param src 
 * @param dest 
 * @return uint8_t* dest
 */
static uint8_t* lzss_finalize(uint32_t runs, const uint8_t* src, uint8_t* dest) {
  while (runs >= MAX_COPY) {
    *dest++ = MAX_COPY - 1;
    lzss_smallcopy(dest, src, MAX_COPY);
    src += MAX_COPY;
    dest += MAX_COPY;
    runs -= MAX_COPY;
  }
  if (runs > 0) {
    *dest++ = runs - 1;
    lzss_smallcopy(dest, src, runs);
    dest += runs;
  }
  return dest;
}
/**
 * @brief return updated op (from short and long match)
 * 
 * @param len 
 * @param distance 
 * @param op 
 * @return uint8_t* 
 */
static uint8_t* lzss1_match(uint32_t len, uint32_t distance, uint8_t* op) {
  --distance;
  if (FASTLZ_UNLIKELY(len > MAX_LEN - 2))
    while (len > MAX_LEN - 2) {
      *op++ = (7 << 5) + (distance >> 8);
      *op++ = MAX_LEN - 2 - 7 - 2;
      *op++ = (distance & 255);
      len -= MAX_LEN - 2;
    }
  if (len < 7) {//short match
    *op++ = (len << 5) + (distance >> 8);
    *op++ = (distance & 255);
  } else { //len==7 long match
    *op++ = (7 << 5) + (distance >> 8);
    *op++ = len - 7;
    *op++ = (distance & 255);
  }
  return op;
}
/**
 * @brief 
 * 
 * @param input   uint8_t 
 * @param length  *char
 * @param output  
 * @return int 
 */
int LZSS1_compress(const void* input, int length, void* output) {
  // input pointer
  const uint8_t* ip = (const uint8_t*)input;
  // start of input pointer
  const uint8_t* ip_start = ip;
  // exclude last 4 bytes
  const uint8_t* ip_bound = ip + length - 4; /* because readU32 */
  const uint8_t* ip_limit = ip + length - 12 - 1;
  // output pointer
  uint8_t* op = (uint8_t*)output;
  
 
  uint32_t htab[HASH_SIZE]; //  seq(3B)-> hash ->ip offset from ip_Start ; hash_size 1<<14

  uint32_t seq, hash;  // 0- (hash_size/1<<14-1)

  /* initializes hash table */
  for (hash = 0; hash < HASH_SIZE; ++hash) htab[hash] = 0;

  /* we start with literal copy */
  const uint8_t* anchor = ip;
  ip += 2;

  /* main loop */
  while (FASTLZ_LIKELY(ip < ip_limit)) {
    const uint8_t* ref;  //pointer to hash
    uint32_t distance, cmp;//cmp: refer to the least three bytes (24 bits)

    /* find potential match */
    do {
      seq = lzss_readu32(ip) & 0xffffff;//keeping 24 least bits
      hash = lzss_hash(seq); //create the hash index
      ref = ip_start + htab[hash];  
      htab[hash] = ip - ip_start; //update the current index to hash table
      distance = ip - ref;   
      cmp = FASTLZ_LIKELY(distance < MAX_L1_DISTANCE) ? lzss_readu32(ref) & 0xffffff : 0x1000000;
      if (FASTLZ_UNLIKELY(ip >= ip_limit)) break;
      ++ip;
    } while (seq != cmp);  // while (seq==cmp) break;  ->Given that htab[hash]=0;  distance=ip-ipstart;

    if (FASTLZ_UNLIKELY(ip >= ip_limit)) break;
    --ip;

    if (FASTLZ_LIKELY(ip > anchor)) {
      op = lzss_literals(ip - anchor, anchor, op);// copy(ip-anchor)times from anchor to op
    }

    uint32_t len = lzss_cmp(ref + 3, ip + 3, ip_bound);
    op = lzss1_match(len, distance, op);

    /* update the hash at match boundary */
    ip += len;
    seq = lzss_readu32(ip);
    hash = lzss_hash(seq & 0xffffff);
    htab[hash] = ip++ - ip_start;
    seq >>= 8;
    hash = lzss_hash(seq);
    htab[hash] = ip++ - ip_start;

    anchor = ip; // -> op
  }//end of while

  uint32_t copy = (uint8_t*)input + length - anchor;
  op = lzss_finalize(copy, anchor, op);//copy anchor to op

  return op - (uint8_t*)output;
}

int LZSS1_decompress(const void* input, int length, void* output, int maxout) {
  const uint8_t* ip = (const uint8_t*)input;
  const uint8_t* ip_limit = ip + length;
  const uint8_t* ip_bound = ip_limit - 2;
  uint8_t* op = (uint8_t*)output;
  uint8_t* op_limit = op + maxout;
  uint32_t ctrl = (*ip++) & 31; //lower 5 bits *(ip++) To Do?

  while (1) {
    if (ctrl >= 32) { //>=2 bytes //Long match && Short match
    //The 3 most-significant bits of opcode[0],
    // M, determines the match length.
      uint32_t len = (ctrl >> 5) - 1;

      uint32_t ofs = (ctrl & 31) << 8;   //update offset to the high bytes of Opcode[0]
       
       //. Note that R is a back reference, 
      //i.e. the value of 0 corresponds
      // the last byte in the output buffer, 
      //1 is the second to last byte, and so forth.
      const uint8_t* ref = op - ofs - 1;  
     
      if (len == 7 - 1) {  // Long match 111
        FASTLZ_BOUND_CHECK(ip <= ip_bound);
        len += *ip++;  //The value of opcode[1], M, determines the match length.
        //The value of 0 indicates a 9-byte match, 1 indicates a 10-byte match
      } 

      //Long match && Short match
      ref -= *ip++; //update offset with Opcode[1] for short match
       //update offset with Opcode[2] fro Long Match

      len += 3;//Short match：(M+2) The value of 1 indicates a 3-byte match, 2 indicates a 4-byte match and so on.
      //The minimum match length is 3 and the maximum match length is 8.
      FASTLZ_BOUND_CHECK(op + len <= op_limit);
      FASTLZ_BOUND_CHECK(ref >= (uint8_t*)output);
      LZSS_memmove(op, ref, len);
      op += len;  //move on


    } else { //Literal run
      ctrl++;//The minimum literal run is 1 and the maximum literal run is 32.
      FASTLZ_BOUND_CHECK(op + ctrl <= op_limit);
      FASTLZ_BOUND_CHECK(ip + ctrl <= ip_limit);
      LZSS_memcpy(op, ip, ctrl); 
      ip += ctrl; //have been copied; move on
      op += ctrl;
    }

    if (FASTLZ_UNLIKELY(ip > ip_bound)) break;
    ctrl = *ip++;
  }

  return op - (uint8_t*)output;
}

static uint8_t* lzss2_match(uint32_t len, uint32_t distance, uint8_t* op) {
  --distance;
  if (distance < MAX_L2_DISTANCE) {
    if (len < 7) {
      *op++ = (len << 5) + (distance >> 8);
      *op++ = (distance & 255);
    } else {
      *op++ = (7 << 5) + (distance >> 8);
      for (len -= 7; len >= 255; len -= 255) *op++ = 255;
      *op++ = len;
      *op++ = (distance & 255);
    }
  } else {
    if (len < 7) {//Level 2 Extended Windows* Short match
      distance -= MAX_L2_DISTANCE;
      *op++ = (len << 5) + 31;
      *op++ = 255;
      *op++ = distance >> 8;
      *op++ = distance & 255;
    } else {//Level 2 Extended Windows* Long match
      distance -= MAX_L2_DISTANCE;
      *op++ = (7 << 5) + 31;
      for (len -= 7; len >= 255; len -= 255) *op++ = 255;
      *op++ = len;
      *op++ = 255;
      *op++ = distance >> 8;
      *op++ = distance & 255;
    }
  }
  return op;
}

int LZSS2_compress(const void* input, int length, void* output) {
  const uint8_t* ip = (const uint8_t*)input;
  const uint8_t* ip_start = ip;
  const uint8_t* ip_bound = ip + length - 4; /* because readU32 */
  const uint8_t* ip_limit = ip + length - 12 - 1;  //?
  uint8_t* op = (uint8_t*)output;

  uint32_t htab[HASH_SIZE];
  uint32_t seq, hash;

  /* initializes hash table */
  for (hash = 0; hash < HASH_SIZE; ++hash) htab[hash] = 0;

  /* we start with literal copy */
  const uint8_t* anchor = ip;//start from the first that haven't been written
  ip += 2;  //Omit the first two char

  /* main loop */
  while (FASTLZ_LIKELY(ip < ip_limit)) {
    const uint8_t* ref;
    uint32_t distance, cmp;

    /* find potential match */
    do {
      seq = lzss_readu32(ip) & 0xffffff; //(p[2] << 16) | (p[1] << 8) | p[0] from current ip store consecutive 3 bytes.
      hash = lzss_hash(seq);
      ref = ip_start + htab[hash];//reference, if not match to the previous then htab=0, ref=ip_start; if match to the previous char[i] , refer to i
      htab[hash] = ip - ip_start; // ip offset from ip_Start (initially 0)
      distance = ip - ref;
      cmp = FASTLZ_LIKELY(distance < MAX_FARDISTANCE) ? lzss_readu32(ref) & 0xffffff : 0x1000000; //if not match to the previous then stick to ip_start; if match, refer to match
      if (FASTLZ_UNLIKELY(ip >= ip_limit)) break;  //no use （assert)
      ++ip;
    } while (seq != cmp); // if not match to the previous,  continue to looping

    if (FASTLZ_UNLIKELY(ip >= ip_limit)) break;
    // if match to the previous one(>=3)
    --ip; // cancel out ++ip;

    /* far, needs at least 5-byte match */
    if (distance >= MAX_L2_DISTANCE) {
      if (ref[3] != ip[3] || ref[4] != ip[4]) {
        ++ip;
        continue;
      }
    }

    if (FASTLZ_LIKELY(ip > anchor)) {
      op = lzss_literals(ip - anchor, anchor, op);
    }

    uint32_t len = lzss_cmp(ref + 3, ip + 3, ip_bound);
    op = lzss2_match(len, distance, op);

    /* update the hash at match boundary */
    ip += len; // move one forward
    seq = lzss_readu32(ip);
    hash = lzss_hash(seq & 0xffffff);
    htab[hash] = ip++ - ip_start; //move the second forward
    seq >>= 8;
    hash = lzss_hash(seq);
    htab[hash] = ip++ - ip_start;

    anchor = ip;
  }

  uint32_t copy = (uint8_t*)input + length - anchor;
  op = lzss_finalize(copy, anchor, op);

  /* marker for LZSS2 */
  *(uint8_t*)output |= (1 << 5);

  return op - (uint8_t*)output;
}

int LZSS2_decompress(const void* input, int length, void* output, int maxout) {
  const uint8_t* ip = (const uint8_t*)input;
  const uint8_t* ip_limit = ip + length;
  const uint8_t* ip_bound = ip_limit - 2;
  uint8_t* op = (uint8_t*)output;
  uint8_t* op_limit = op + maxout;
  uint32_t ctrl = (*ip++) & 31;

  while (1) {
    if (ctrl >= 32) { //Level 2 Extended Windows* Short match
      uint32_t len = (ctrl >> 5) - 1;
      uint32_t ofs = (ctrl & 31) << 8;
      const uint8_t* ref = op - ofs - 1;

      uint8_t code;
      if (len == 7 - 1) do { //Level 2 Extended Windows* Long match
          FASTLZ_BOUND_CHECK(ip <= ip_bound);
          code = *ip++;
          len += code;
        } while (code == 255);  //Len left or Windows size not full
      code = *ip++;
      ref -= code;
      len += 3;

      /* match from 16-bit distance */
      if (FASTLZ_UNLIKELY(code == 255))
        if (FASTLZ_LIKELY(ofs == (31 << 8))) {
          FASTLZ_BOUND_CHECK(ip < ip_bound);
          ofs = (*ip++) << 8;
          ofs += *ip++;
          ref = op - ofs - MAX_L2_DISTANCE - 1;
        }

      FASTLZ_BOUND_CHECK(op + len <= op_limit);
      FASTLZ_BOUND_CHECK(ref >= (uint8_t*)output);
      LZSS_memmove(op, ref, len);
      op += len;
    } else {
      ctrl++;
      FASTLZ_BOUND_CHECK(op + ctrl <= op_limit);
      FASTLZ_BOUND_CHECK(ip + ctrl <= ip_limit);
      LZSS_memcpy(op, ip, ctrl);
      ip += ctrl;
      op += ctrl;
    }

    if (FASTLZ_UNLIKELY(ip >= ip_limit)) break;
    ctrl = *ip++;
  }

  return op - (uint8_t*)output;
}
/**
  DEPRECATED.

  This is similar to LZSS_compress_level above, but with the level
  automatically chosen.

  This function is deprecated and it will be completely removed in some future
  version.
*/
// int LZSS_compress(const void* input, int length, void* output) {
//   /* for short block, choose LZSS1 */
//   if (length < 65536) return LZSS1_compress(input, length, output);

//   /* else... */
//   return LZSS2_compress(input, length, output);
// }


/**
 * @brief 
 * Decompress a block of compressed data and returns the size of the
  decompressed block. If error occurs, e.g. the compressed data is
  corrupted or the output buffer is not large enough, then 0 (zero)
  will be returned instead.

  The input buffer and the output buffer can not overlap.

  Decompression is memory safe and guaranteed not to write the output buffer
  more than what is specified in maxout.

  Note that the decompression will always work, regardless of the
  compression level specified in LZSS_compress_level above (when
  producing the compressed block).
 */
int LZSS_decompress(const void* input, int length, void* output, int maxout) {
  /* magic identifier for compression level */
  int level = ((*(const uint8_t*)input) >> 5) + 1;

  if (level == 1) return LZSS1_decompress(input, length, output, maxout);
  if (level == 2) return LZSS2_decompress(input, length, output, maxout);

  /* unknown level, trigger error */
  return 0;
}
/**
 * @brief 
    Compress a block of data in the input buffer and returns the size of
  compressed block. The size of input buffer is specified by length. The
  minimum input buffer size is 16.

  The output buffer must be at least 5% larger than the input buffer
  and can not be smaller than 66 bytes.

  If the input is not compressible, the return value might be larger than
  length (input buffer size).

  The input buffer and the output buffer can not overlap.

  Compression level can be specified in parameter level. At the moment,
  only level 1 and level 2 are supported.
  Level 1 is the fastest compression and generally useful for short data.
  Level 2 is slightly slower but it gives better compression ratio.

  Note that the compressed data, regardless of the level, can always be
  decompressed using the function LZSS_decompress below.
 * @param level 
 * 1 -> LZSS1_compress(input, length, output) | 
 * 2->LZSS2_compress(input, length, output)
 * @return int chunk_size (size_t __nitems nums of output) 
 */
int LZSS_compress_level(int level, const void* input, int length, void* output) {
  if (level == 1) return LZSS1_compress(input, length, output);
  if (level == 2) return LZSS2_compress(input, length, output);
  return 0;
}

int main(){
  //------------Compress---------------
     clock_t tStart = clock();
    FILE* infile =fopen("2CylinderEngine.obj","r");
    //FILE* infile =fopen("2CylinderEngine.obj","r");
    /* Get the number of bytes */
    
     fseek(infile, 0L, SEEK_END);
     const long long numbytes = ftell(infile);
    //std::cout<<numbytes;
    // /* reset the file position indicator to 
    // the beginning of the file */
     fseek(infile, 0L, SEEK_SET);
    // read_chunk_header(infile,  &numbytes,  &chunk_extra);
    //unsigned char* buffer[num_threads];
    long long each_numbytes=numbytes/num_threads;
    const long long last_numbytes=each_numbytes+numbytes%num_threads;
    assert(each_numbytes*(num_threads-1)+last_numbytes==numbytes);
    assert(last_numbytes>=each_numbytes);
    unsigned char buffer[num_threads][last_numbytes];
    unsigned char buffer2[num_threads][last_numbytes];
    const int last_i=num_threads-1;
  //  for(int i=0;i<num_threads-1;i++){
  //     buffer[i] = new unsigned char[numbytes];
  //  }
    // buffer[num_threads-1]=new unsigned char[numbytes];
    //unsigned char* buffer_rev = new unsigned char[numbytes];
 
   
    for(int tid=0;tid<num_threads-1;tid++){
      fread(buffer[tid], sizeof(char), each_numbytes, infile);
    }
    fread(buffer[last_i], sizeof(char), last_numbytes, infile);
    fclose(infile);
    // cout<<count<<endl<<numbytes;
    // FILE *f4=fopen("C2.txt","wb");
    //  fwrite(buffer, 1, numbytes , f4);
    //  fclose(f4);

    long long chunk_size[num_threads];
    for(int tid=0;tid<num_threads-1;tid++){  
      chunk_size[tid]=LZSS_compress_level(1,buffer[tid], each_numbytes, buffer2[tid]);
    }
    chunk_size[last_i]=LZSS_compress_level(1,buffer[last_i], last_numbytes, buffer2[last_i]);
    
    //std::cout<<numbytes<<chunk_size<<std::endl;
    FILE *f=fopen("output2.txt","wb");
    //std::cout<<buffer2[0];
      // cout<<chunk_size;
      long long count_total=0;
    for(int tid=0;tid<num_threads-1;tid++){  
      fwrite(buffer2[tid], 1, chunk_size[tid], f);
      count_total+=chunk_size[tid];
    }
    fwrite(buffer2[last_i], 1, chunk_size[last_i], f);
    count_total+=chunk_size[last_i];
    cout<<count_total;
    fclose(f);
    // for(int tid=0;tid<num_threads;tid++){
    //     delete buffer[tid];
    //     delete buffer2[tid];
    // }
    
    //debug_readin();

  
  //------------Decompress---------------
    FILE* infile2 =fopen("output2.txt","rb");
    long long chunk_extra=numbytes;
    /* Get the number of bytes */
    //  fseek(infile2, 0L, SEEK_END);
    // const long long numbytes2 = ftell(infile2);
    
    /* reset the file position indicator to 
     the beginning of the file */
     fseek(infile2, 0L, SEEK_SET);
   
 
    // unsigned char* buffer3 = new unsigned char[numbytes2];
    // unsigned char* buffer4 = new unsigned char[chunk_extra+10];
    const long long second_arg=1000000;
    unsigned char buffer3[num_threads][second_arg];//!!To Do Jacob: After Decompression, file becomes bigger
    unsigned char buffer4[num_threads][second_arg];


    for(int tid=0;tid<num_threads-1;tid++){
      fread(buffer3[tid], sizeof(char), chunk_size[tid], infile2);
    }
    fread(buffer3[last_i], sizeof(char), chunk_size[last_i], infile2);
    long long original_size[num_threads];
    for(int tid=0;tid<num_threads-1;tid++){
        original_size[tid]=each_numbytes ;
    }
    original_size[last_i]=last_numbytes;
    // FILE *f2=fopen("Decompressed.txt","wb");
    // fwrite(buffer3, 1, numbytes2 , f2);
    // fclose(f2);
  FILE *f2=fopen("Decompressed.txt","w");


   vector<thread> threads;
     long long chunk_size2[num_threads]; 
    //one-D array
  
    // unsigned char buffer3_one[num_threads*last_numbytes];//!!To Do Jacob: After Decompression, file becomes bigger
    // unsigned char buffer4[num_threads*last_numbytes];   

  //    for(int row=0;row<num_threads;row++){
  //   for(int col=0;col<last_numbytes;col++){
  //       buffer3_one[row*last_numbytes+col]=buffer3[row][col];
  //   }
  // }
    // auto lambda=[&buffer3_one,&buffer4,&last_numbytes,&chunk_size,&chunk_size2,& original_size,&f2](int tid){
    //   chunk_size2[tid] =LZSS_decompress((void*)buffer3_one[tid],chunk_size[tid] , (void*) buffer4[tid*last_numbytes],original_size[tid]); 
    //   //assert(chunk_size2[tid]==original_size[tid]);
    //   fwrite((void*)buffer4[tid*last_numbytes], 1, original_size[tid] , f2);
    // };  

    //two-D array
    // unsigned char buffer3[num_threads][last_numbytes];//!!To Do Jacob: After Decompression, file becomes bigger
    // unsigned char buffer4[num_threads][last_numbytes];

    // auto lambda=[&buffer3,&buffer4,&last_numbytes,&chunk_size,&chunk_size2,& original_size,&f2](int tid){
      auto lambda=[&](int tid){
      chunk_size2[tid] =LZSS_decompress(buffer3[tid],chunk_size[tid] , buffer4[tid],original_size[tid]); 
      //assert(chunk_size2[tid]==original_size[tid]);
      fwrite(buffer4[tid], 1, original_size[tid] , f2);
    };  

    
    for(int tid=0;tid<num_threads;tid++){
       threads.push_back(thread(lambda,tid));
    }
     for(int tid=0;tid<num_threads;tid++){
        threads[tid].join();
    }
    // for(int tid=0;tid<num_threads;tid++){
    //    chunk_size2[tid] =LZSS_decompress(buffer3[tid],chunk_size[tid] , buffer4[tid],original_size[tid]); 
    //    assert(chunk_size2[tid]==original_size[tid]);
    //    fwrite(buffer4[tid], 1, original_size[tid] , f2);
    // }
    // chunk_size2[last_i] =LZSS_decompress(buffer3[last_i],chunk_size[last_i] , buffer4[last_i],last_numbytes);
    // //std::cout<<numbytes<<chunk_extra<<chunk_size<<endl;
    //  assert(chunk_size2[last_i]==last_numbytes);
    // fwrite(buffer4[last_i], 1, last_numbytes , f2);
    fclose(f2);
    // delete[]buffer3;
    // delete[]buffer4;
    fclose(infile2);
         /* Do your stuff here */
    printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
 
    return 0;

}