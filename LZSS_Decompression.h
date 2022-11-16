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

class LZSS_Decoder{
  private:
         void* INPUT_; 
      int LENGTH_;  //length of the block of data to be Decompressed
      void* OUTPUT_;
         ubyte* input; // pointer to input data
         ubyte* input_limit; // input data size (max)
      ubyte* output ; // pointer to output data
      uint ctrl;  // current position's value
         ubyte* ref; 
      uint match_len;
      ull offset;// distance from current position to references
      int length_after;
  public:
    LZSS_Decoder(   void* INPUT, int length, void* OUTPUT):INPUT_(INPUT), input((   ubyte*)INPUT){
      this->LENGTH_=length;
      input_limit = input + length;
      ctrl = (*input++) & 31;//start with literal run, 000 L4-L0 lower 5 bits -> length of the literal run *(input++)  
      this->OUTPUT_=OUTPUT;
      output = (ubyte*)OUTPUT_;
      length_after=0; //default / failed
    }
    ~LZSS_Decoder(){}
    int Decompress(){
      // marker for compression level 
      int level =((*(   ubyte*)INPUT_) >>6)<<1)+ (((*(   ubyte*)INPUT_) >>7 ) <<1) + 1;
      if (level == 1) {
        level1();
     
      }
      else if (level == 2) {
        level2();
      }
      else if(level>=3){
        level3();
      }
      //level !=2| level !=1, return 0
      return length_after; 
    }
      
  public:
      /* Copies the values of count bytes from the location pointed by source to 
    the memory block pointed by destination. Copying takes place as if an 
    intermediate buffer were used, allowing the destination and source to overlap.
    64-bit implementation for speed improvements.
    memmove=copy8 */
    void mem_move(ubyte* destination,    ubyte* source, uint count) {
      if ((count > 4) && (destination >= source + count)) {
        memmove(destination, source, count);
      } else {
          for(uint i=0;i<count;i++){ // overlapping area
            *destination++ = *source++;
          }
      }
      // if (count) [[likely]] {
      // if (destination >= source + count || destination + count <= source) {
      //   memcpy(destination, source, count);
      // } else {
      //   do {
      //     *destination++ = *source++;
      //   } while(--count);
      // }
    }
    void level1() {
      while (input <= input_limit - 2)[[likely]] {
        if (ctrl >= 32) { //>=2 bytes //Long match && Short match
        //The 3 most-significant bits of opcode[0],
        // M, determines the match length.
        match_len = (ctrl >> 5) - 1;
        offset = (ctrl & 31) << 8;//update offset to the high bytes of Opcode[0]
          
          //. Note that R is a back reference, 
          //i.e. the value of 0 corresponds
          // the last byte in the output buffer, 
          //1 is the second to last byte, and so forth.
          ref = output - offset - 1;
          if (match_len == 7 - 1) { // Long match 111
            match_len += *input++; //The value of opcode[1], M, determines the match length.
            //The value of 0 indicates a 9-byte match, 1 indicates a 10-byte match
          }
          //Long match && Short match
          ref -= *input++;//update offset with Opcode[1] for short match
          //update offset with Opcode[2] fro Long Match
          match_len += 3;//Short match：(M+2) The value of 1 indicates a 3-byte match, 2 indicates a 4-byte match and so on.
          //The minimum match length is 3 and the maximum match length is 8.
          this->mem_move(output, ref, match_len);
          output += match_len;//move on
        } else { //Literal run
          ctrl++;//The minimum literal run is 1 and the maximum literal run is 32.
          memcpy(output, input, ctrl);
          input += ctrl;//have been copied; move on
          output += ctrl;
        }
        ctrl = *input++;
      }
      length_after =output - (ubyte*)OUTPUT_;
    }

    void level2() {
      while (input < input_limit)[[likely]] {
        if (ctrl >= 0b00100000) { //Not lieteral run Level 2 Extended Windows* Short match
          match_len = (ctrl >> 5) - 1;  // M2-M0 high 3 bits
          offset = (ctrl & 31) << 8;    // W12-W8 low 5 bits
          ref = output - offset - 1;    // reference (copied) of existed decoded output
          ubyte code;  //current position's value
          if (match_len == 7 - 1) do {//Level 2 Long match: read in all of the len
              code = *input++;  //...... M7-M0
              match_len += code;
            } while (code == 255);//Len left or Windows size not full
          code = *input++;  //W7-W0 
          //For Extended Windows | Short match code=255
          ref -= code;
          match_len += 3;

          /* match from 16-bit distance */
          if (code == 255) [[unlikely]]//level 2 Extended Windows 
            if (offset == (31 << 8))[[likely]] { //Distinguish with level1
              offset = (*input++) << 8;  // 1111 1111  |  W15-W8   |   W7-W0  
              offset += *input++;
              ref = output - offset - MAX_L2_Length - 1;
            }
          this->mem_move(output, ref, match_len);
          output += match_len;
        } else { //literal run 
          ctrl++; //start with 0
          memcpy(output, input, ctrl);
          input += ctrl;
          output += ctrl;
        }
        ctrl = *input++;
      }
      length_after=output - (ubyte*)OUTPUT_;
    }
    long long level3() {
      // bool is_longMatch;
      while (input < input_limit)[[likely]] {
        if (ctrl >= 32) { //Level 2 Extended Windows* Short match
        if((input-(ubyte*)INPUT_)>=143775)
        {
            int a=1;
        }
          match_len = (ctrl >> 5) - 1;
          // is_longMatch=false;
          offset = (ctrl & 31) << 8;
          ref = output - offset - 1;
          ubyte code;
          if (match_len == 7 - 1) do {//Level 2 Extended Windows* Long match
              // is_longMatch=true;
              code = *input++;    
              match_len += code;
            } while (code == 255);//Len left or Windows size not full
          code = *input++; //|-
          ref -= code;
          match_len += 3;
// | Extended Windows*| ----------------------------------------------------------------------------------------|
// | Short match      | M2-M0 11111 |- 1111 1111 |$    W15-W8    |   W7-W0   |           |           |           |
// | Long match       | 111   11111 |  M7-M0...  |-  1111 1111  |$  W15-W8   |   W7-W0   |     	  	 |           |
// | Extended Windows2| ----------------------------------------------------------------------------------------|
// | Extra Short match| M2-M0 11111 |- 1111 1111 |$   1111 1111  | 1111 1111 |  W15-W8   |   W7-W0   |           | 
// | Extra Long match | 111   11111 |  M7-M0...  |-  1111 1111  |$ 1111 1111 | 1111 1111 |   W15-W8	 |   W7-W0   |
          /* match from 32-bit distance */
          if (code == 255) [[unlikely]]{
            if (offset == (31 << 8))[[likely]] {  //level 2-3
              offset = (*input++) << 8; //|$
              offset += *input++;                 //  W7-W0  
              if(offset != ((1<<16)-1))[[likely]]{ //level 2 1111 1111 |   W15-W8	 |   W7-W0          
                ref = output - offset - MAX_L2_Length - 1;
              }
              else{ //level3 1111 1111  | 1111 1111 | 1111 1111 |   W15-W8	 |   W7-W0  
                offset = (*input++) << 8; /// W15-W8
                offset += *input++;//  W7-W0
                ref = output - offset -65535- MAX_L2_Length-1; 
                //ref-(ubyte*)OUTPUT_;    
              }
            }
          }
          this->mem_move(output, ref, match_len);
          output += match_len;
        } else {
          ctrl++;
          memcpy(output, input, ctrl);
          input += ctrl;
          output += ctrl;
        }
        ctrl = *input++;
      }
      length_after=output - (ubyte*)OUTPUT_;
      return length_after;
    }

};
