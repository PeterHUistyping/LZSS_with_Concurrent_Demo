/* 

  Compression and Decompression Using LZSS Algorithm
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
#include "LZSS_helper.h"
/**
 * @brief *destination++ = COPY_MAX - 1;
 * @param runs numbers of byte to be written
 * @param source  start point  
 * @param destination
 * @return uint8_t* 
 */
static ubyte* Literals_Output(uint32_t runs, const ubyte* source, ubyte* destination) {
  while (runs >= 32) { //COPY_MAX
    *destination++ = 32 - 1;
    lzss_copy256(destination, source);
    source += 32;
    destination += 32;
    runs -= 32;
  }
  if (runs > 0) {
    *destination++ = runs - 1;
    lzss_copy64(destination, source, runs);
    destination += runs;
  }
  return destination;
}
/**
 * @brief copy source -> destination
 * LZSS_memcpy(destination, source, count);  copy the second arg -> first arg
 * 
 * @param runs copy <=COPY_MAX
 * @param source 
 * @param destination 
 * @return uint8_t* destination
 */
static ubyte* lzss_finalize(uint32_t runs, const ubyte* source, ubyte* destination) {
  while (runs >= 32) { //COPY_MAX
    *destination++ = 32 - 1;
    lzss_smallcopy(destination, source, 32);
    source += 32;
    destination += 32;
    runs -= 32;
  }
  if (runs > 0) {
    *destination++ = runs - 1;
    lzss_smallcopy(destination, source, runs);
    destination += runs;
  }
  return destination;
}
/**
 * @brief return updated output (from short and long match)
 * 
 * @param len 
 * @param distance 
 * @param output 
 * @return uint8_t* 
 */
static ubyte* Match_Output1(uint32_t len, uint32_t distance, ubyte* output) {
  --distance;
  if (len > 264 - 2)[[unlikely]]  //Max of match length 256+8
    while (len > 264 - 2) {
      *output++ = (7 << 5) + (distance >> 8);
      *output++ = 264 - 2 - 7 - 2;
      *output++ = (distance & 255);
      len -= 264 - 2;
    }
  if (len < 7) {//short match
    *output++ = (len << 5) + (distance >> 8);
    *output++ = (distance & 255);
  } else {//len==7 long match
    *output++ = (7 << 5) + (distance >> 8);
    *output++ = len - 7;
    *output++ = (distance & 255);
  }
  return output;
}

static ubyte* Match_Output2(uint32_t len, uint32_t distance, ubyte* output) {
  --distance;
  if (distance < MAX_L2_Length) {
    if (len < 7) {
      *output++ = (len << 5) + (distance >> 8);
      *output++ = (distance & 255);
    } else {
      *output++ = (7 << 5) + (distance >> 8);
      for (len -= 7; len >= 255; len -= 255) *output++ = 255;
      *output++ = len;
      *output++ = (distance & 255);
    }
  } else {
    /* still far away... */
    if (len < 7) {//Level 2 Extended Windows* Short match
      distance -= MAX_L2_Length;
      *output++ = (len << 5) + 31;
      *output++ = 255;
      *output++ = distance >> 8;
      *output++ = distance & 255;
    } else {//Level 2 Extended Windows* Long match
      distance -= MAX_L2_Length;
      *output++ = (7 << 5) + 31;
      for (len -= 7; len >= 255; len -= 255) *output++ = 255;
      *output++ = len;
      *output++ = 255;
      *output++ = distance >> 8;
      *output++ = distance & 255;
    }
  }
  return output;
}
class HashTable{
  public:
    uint32_t table[HASH_SIZE]; //  sequence (every 3 B) -> hash 
    HashTable(){
        for (uint32_t hash = 0; hash < HASH_SIZE; ++hash){
          table[hash] = 0;
        }  
    }

  /**
   * @brief Creating the hash index for the sequence given
   * @param seq 3 bytes sequence given
   * @return uint16_t the hash index
   */
  static uint16_t hash_func(uint32_t seq) {
    uint32_t hash = (seq * 2654435769LL) >> (32 - 14); //HASH_LOG=14
    return hash & Hash_Key_Masks;   
  }
};

class LZSS_Encoder{
  private:
      const void* INPUT_; 
      int LENGTH_;  //length of the block of data to be compressed
      void* OUTPUT_;
      const ubyte* input; // pointer to input data
      ubyte* output ; // pointer to output data

      const ubyte* input_start;
      const ubyte* input_bound;  
      const ubyte* input_limit;  //?

      HashTable Hash; //initialized
      uint32_t hash; // 0  - {(hash_size/1<<14)-1}
      const ubyte* pivot ;// first of unwritten byte
      const ubyte* ref; //pointer to ubyte (starting point of three bytes) of the same three bytes given
      uint32_t sequence;// a sequence of 3 bytes from the third of the input data given.(For the first two, no need to implement)
    
      uint32_t distance; //offset from current position to the reference 
      uint32_t cmp; //cmp ref with sequence given
      uint32_t match_len;
      uint32_t last_num_bytes;//last several literals
      int length_after;

  public:
    LZSS_Encoder(const void* INPUT, int length, void* OUTPUT_):INPUT_(INPUT), input((const ubyte*)INPUT),Hash(){
      this->LENGTH_=length;
      this->OUTPUT_=OUTPUT_;
      length_after=0;
      output = (ubyte*)OUTPUT_;
      pivot = input;
      input_start = input;
      input_bound = input + LENGTH_ - 4; /* because readU32 */
      input_limit = input + LENGTH_ - 12 - 1;  //?

      input += 2;//Omit the first two char
    }
    
    int Compress(){
      /* for short block, choose level1 */
      if (LENGTH_ < LEVEL1_MAX) {
        this->level1();
        return length_after; 
      }
      /* else length >  LEVEL1_MAX */
        this->level2();
        return length_after;
    }
    int Compress(int level){
      if (level==1) {
        this->level1();
        return length_after; 
      }
      else if (level==2){
        this->level2();
        return length_after;
      }
    }
    private:
      void level1() {
        // literal copy
        while (input < input_limit)[[likely]] {
          // find potential match  
          while(input <= input_limit) [[likely]]{
            sequence = read_uint32(input) & 0xffffff; //take every three bytes as a sequence  
            hash = HashTable::hash_func(sequence);//create the hash index
            ref = input_start + Hash.table[hash];  //input_start+ uint16
            Hash.table[hash] = input - input_start;//update the current index to hash table
            distance = input - ref; //offset
            cmp = (__builtin_expect(!!(distance < WINDOW_SIZE_L1), 1))  ? read_uint32(ref)  & 0xffffff : 0x1000000; 
            //check distance < WINDOW SIZE 
            //GNU Extension //likely
            if (sequence == cmp) break;   
            ++input;  //move on 1 byte to take a new sequence
          }   
          if (input >= input_limit) [[unlikely]]break;
          //write literal run part to the output
          if (input > pivot) [[likely]]{                          
            output = Literals_Output(input - pivot, pivot, output); // copy(input-pivot)times from pivot to output
          }
          //write short/long match part to the output
          match_len = match_cmp(ref + 3, input + 3, input_bound); 
          //ignore compared part and compare until input_bound (EOF)
          output = Match_Output1( match_len, distance, output);

          input +=  match_len;  //for the match boundary, the second byte of the last matched sequence 
          sequence = read_uint32(input);   // update hash table
          hash =HashTable::hash_func(sequence);
          Hash.table[hash] = input++ - input_start; //the third byte of the last matched sequence 
          sequence >>= 8;
          hash = HashTable::hash_func(sequence);
          Hash.table[hash] = input++ - input_start; //first of unwritten byte

          pivot = input; // -> updated first of unwritten byte
        }/* end of while */

        last_num_bytes = (ubyte*)INPUT_ + LENGTH_ - pivot;// the rest of unmatched part
        output = lzss_finalize(last_num_bytes, pivot, output);//copy from pivot to EOF to output

        length_after= output - (ubyte*)OUTPUT_;
}

    void level2() {
      while (input < input_limit)[[likely]] {
        // potential match
        while(input <= input_limit)[[likely]]{
          sequence = read_uint32(input) & 0xffffff;
          //(p[2] << 16) | (p[1] << 8) | p[0] from current input store consecutive 3 bytes.
          hash = HashTable::hash_func(sequence);
          ref = input_start + Hash.table[hash];
          //reference, if not match to the previous then HashTable=0, ref=input_start; if match to the previous char[i], refer to i
          Hash.table[hash] = input - input_start;// input offset from ip_Start (initially 0)
          distance = input - ref;
          cmp = (__builtin_expect(!!(distance < (65535 + MAX_L2_Length - 1)), 1)) ? read_uint32(ref) & 0xffffff : 0x1000000; 
          //if not match to the previous then stick to input_start; if match, refer to match
          if(sequence == cmp)  break;
          ++input;
        } // if not match to the previous,  continue to looping

        if (input >= input_limit) [[unlikely]]break;
          // if match to the previous one(>=3)

        /* far, needs at least 5-byte match */
        if (distance >= MAX_L2_Length) {
          if (ref[3] != input[3] || ref[4] != input[4]) {
            ++input;
            continue;
          }
        }

        if (input > pivot) [[likely]]{
          output = Literals_Output(input - pivot, pivot, output);
        }

        match_len = match_cmp(ref + 3, input + 3, input_bound);
        output = Match_Output2( match_len, distance, output);

        /* update the hash at match boundary */
        input +=  match_len;// move one forward
        sequence = read_uint32(input);
        hash = HashTable::hash_func(sequence & 0xffffff);
        Hash.table[hash] = input++ - input_start; //move the second forward
        sequence >>= 8;
        hash = HashTable::hash_func(sequence);
        Hash.table[hash] = input++ - input_start;

        pivot = input;
      }

      last_num_bytes = (ubyte*)INPUT_ + LENGTH_ - pivot;
      output = lzss_finalize (last_num_bytes, pivot, output);

      /* marker for LZSS2 */
      *(ubyte*)OUTPUT_ |= (1 << 5);

      length_after= output - (ubyte*)OUTPUT_;
    }
};
