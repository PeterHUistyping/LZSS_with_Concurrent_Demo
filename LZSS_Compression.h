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
class HashTable{
  public:
    uint table[HASH_SIZE]; //  sequence (every 3 B) -> hash 
    HashTable(){
        for (uint hash = 0; hash < HASH_SIZE; ++hash){
          table[hash] = 0;
        }  
    }

  /**
   * @brief Creating the hash index for the sequence given
   * @param seq 3 bytes sequence given
   * @return uint16_t the hash index
   */
  static uint16_t hash_func(uint seq) {
    uint hash = (seq * 2654435769LL) >> (32 - 14); //HASH_LOG=14
    return hash & Hash_Key_Masks;   
  }
};

class LZSS_Encoder{
  private:
    void* INPUT_; 
    int LENGTH_;  //length of the block of data to be compressed
    void* OUTPUT_;
      ubyte* input; // pointer to input data
      ubyte* input1; // pointer to input data
    ubyte* output ; // pointer to output data

      ubyte* input_start;
      ubyte* input_bound;  
      ubyte* input_limit;  //?

    HashTable Hash; //initialized
    uint hash; // 0  - {(hash_size/1<<14)-1}
      ubyte* pivot ;// first of unwritten byte
    ubyte* ref; //pointer to ubyte (starting point of three bytes) of the same three bytes given
    ubyte* ref1;
    uint sequence;// a sequence of 3 bytes from the third of the input data given.(For the first two, no need to implement)
  
    uint distance; //offset from current position to the reference 
    uint cmp; //cmp with sequence given
    uint match_len;
    uint last_num_bytes;//last several literals
    int Window_Num; //level4
    int length_after;

  public:
    LZSS_Encoder(void* INPUT, int length, void* OUTPUT_): input((  ubyte*)INPUT),Hash(){
      INPUT_=INPUT;
      this->LENGTH_=length;
      this->OUTPUT_=OUTPUT_;
      length_after=0;
      output = (ubyte*)OUTPUT_;
      pivot = input;
      input_start = input;
      input_bound = input + LENGTH_ - 4; /* because readU32 */
      input_limit = input + LENGTH_ - 12 - 1;  //?
      match_len=0;
      input += 2;//Omit the first two char
      Window_Num=2;
      // Window_Num=(input_limit-input)/65535;//level4
    }
    ~LZSS_Encoder(){}
    int Compress(){
      /* for short block, choose level1 */
      if (LENGTH_ < LEVEL1_MAX) {
        this->level2();
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
      else if (level==3){
        this->level3();
        return length_after;
      }
      return 0;
    }
    int getWindowSize(){ //level4
      return Window_Num;
    }
    public:
      /* Copies the values of (count+1) bytes (MAX OF 256 bytes) from the location pointed by source to 
      the memory block pointed by destination.*/
      void copy(ubyte* destination,   ubyte* source, uint count) {
          ull* src = (  ull*)source;//64Bytes
        ull* dest = (ull*)destination;
        *dest++ = *src++; //0-7
        if(count<16){ //8-15
          *dest++ = *src++;
        }else if(count>=24){//24-31
          *dest++ = *src++;
          *dest++ = *src++;
          *dest++ = *src++;
        }else{//16-23
          *dest++ = *src++;
          *dest++ = *src++;
        }
      }
      /**
       * @brief *destination++ = COPY_MAX - 1;
       * @param runs numbers of bytes to be written
       * @param source  start point  
       * @param destination
       * @return uint8_t* 
       */
      ubyte* Literals_Output(uint runs,   ubyte* source, ubyte* destination) {
        while (runs >0 ) { //COPY_MAX    32*8
          uint this_run =runs;
          if(runs>32) {
            this_run=32;
          }
          *destination++ = this_run - 1;
          this->copy(destination, source,this_run);
          source += this_run;
          destination += this_run;
          runs -= this_run;
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
      ubyte* Match_Output1(uint len, uint distance, ubyte* output) {
        --distance;
        uint index=0;
        if (len > 264 - 2)[[unlikely]]  //Max of match length 256+8
          while (len > 264 - 2) {
            output[index++] = (7 << 5) + (distance >> 8);
            output[index++] = 264 - 2 - 7 - 2;
            output[index++] = (distance & 0xff);
            len -= 264 - 2;
          }
        if (len < 7) {//short match
          output[index++] = (len << 5) + (distance >> 8);
          output[index++] = (distance & 0xff);
        } else {//len==7 long match
          output[index++] = (7 << 5) + (distance >> 8);
          output[index++] = len - 7;
          output[index++] = (distance & 0xff);
        }
        return &output[index];
      }

      ubyte* Match_Output2(uint len, uint distance, ubyte* output) {
        --distance;
        uint index=0;
        if (distance < MAX_L2_Length) {
          if (len < 7) {
            output[index++] = (len << 5) + (distance >> 8);
            output[index++] = (distance & 0xff);
          } else {
            output[index++] = (7 << 5) + (distance >> 8);
            for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
            output[index++] = len;
            output[index++] = (distance & 0xff);
          }
        } else {
          /* still far away... */
          if (len < 7) {//Level 2 Extended Windows* Short match
            distance -= MAX_L2_Length;
            output[index++] = (len << 5) + 0x1f;  //0b111 11111
            output[index++] = 0xff;
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          } else {//Level 2 Extended Windows* Long match
            distance -= MAX_L2_Length;
            output[index++] = (7 << 5) + 0x1f;  //0b111 11111
            for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
            output[index++] = len;   //M7-M0...
            output[index++] = 0xff;  //0b 1111 1111
            output[index++] = distance >> 8;  //W15-W8
            output[index++] = distance & 0xff; // W7-W0 
          }
        }
        return &output[index];
      }
      ubyte* Match_Output3(uint len, uint distance, ubyte* output) {
        --distance;
        uint index=0;
        if (distance < MAX_L2_Length) {
          if (len < 7) {
            output[index++] = (len << 5) + (distance >> 8);
            output[index++] = (distance & 0xff);
          } else {
            output[index++] = (7 << 5) + (distance >> 8);
            for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
            output[index++] = len;
            output[index++] = (distance & 0xff);
          }
        } else if(distance<MAX_L2_Length+65535){
          /* still far away... */
          if (len < 7) {//Level 2 Extended Windows* Short match
            distance -= MAX_L2_Length;
            output[index++] = (len << 5) + 0x1f;  
            output[index++] = 0xff;
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          } else {//Level 2 Extended Windows* Long match
            distance -= MAX_L2_Length;
            output[index++] = (7 << 5) + 0x1f;
            for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
            output[index++] = len;
            output[index++] = 0xff;
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          }
        } else{ //level3
            if (len < 7) {//Level 3 Extended Windows* Short match
            distance -= MAX_L2_Length;          
             distance -= 65535;
            output[index++] = (len << 5) + 0x1f;
            output[index++] = 0xff;
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          } else {//Level 3 Extended Windows* Long match
            distance -= MAX_L2_Length;
            distance -= 65535;
            output[index++] = (7 << 5) + 0x1f;
            for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
            output[index++] = len;
            output[index++] = 0xff;
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          }
        }
        return &output[index];
      }
       ubyte* Match_Output4(uint len, uint distance, ubyte* output) {
        --distance;
        uint index=0;
        //  if (distance < MAX_L2_Length) {
        //   if (len < 7) {
        //     output[index++] = (len << 5) + (distance >> 8);
        //     output[index++] = (distance & 0xff);
        //   } else {
        //     output[index++] = (7 << 5) + (distance >> 8);
        //     for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
        //     output[index++] = len;
        //     output[index++] = (distance & 0xff);
        //   }
        // } else if(distance<MAX_L2_Length+65535){
        //   /* still far away... */
        //   if (len < 7) {//Level 2 Extended Windows* Short match
        //     distance -= MAX_L2_Length;
        //     output[index++] = (len << 5) + 0x1f;  
        //     output[index++] = 0xff;
        //     output[index++] = distance >> 8;
        //     output[index++] = distance & 0xff;
        //   } else {//Level 2 Extended Windows* Long match
        //     distance -= MAX_L2_Length;
        //     output[index++] = (7 << 5) + 0x1f;
        //     for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
        //     output[index++] = len;
        //     output[index++] = 0xff;
        //     output[index++] = distance >> 8;
        //     output[index++] = distance & 0xff;
        //   }
        // } else{ //level3
        //     if (len < 7) {//Level 3 Extended Windows* Short match
        //     distance -= MAX_L2_Length;          
        //      distance -= 65535;
        //     output[index++] = (len << 5) + 0x1f;
        //     output[index++] = 0xff;
        //     output[index++] = 0xff; //0b 1111 1111
        //     output[index++] = 0xff; //0b 1111 1111
        //     output[index++] = distance >> 8;
        //     output[index++] = distance & 0xff;
        //   } else {//Level 3 Extended Windows* Long match
        //     distance -= MAX_L2_Length;
        //     distance -= 65535;
        //     output[index++] = (7 << 5) + 0x1f;
        //     for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
        //     output[index++] = len;
        //     output[index++] = 0xff;
        //     output[index++] = 0xff; //0b 1111 1111
        //     output[index++] = 0xff; //0b 1111 1111
        //     output[index++] = distance >> 8;
        //     output[index++] = distance & 0xff;
        //   }
        // }
        if (len < 7) {
            output[index++] = (len << 5) + (distance >> 8);
            output[index++] = (distance & 0xff);
          } else {
            output[index++] = (7 << 5) + (distance >> 8);
            for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
            output[index++] = len;
            output[index++] = (distance & 0xff);
          }
        for(int i=1;i<=Window_Num;i++){
            if (distance <i*65535+ MAX_L2_Length) {
              distance -= MAX_L2_Length;          
              distance -= 65535*i;
            if (len < 7) {
              output[index++] = (len << 5) + 0x1f;
              output[index++] = 0xff;
              for(int j=2;j<=i;j++){
                output[index++] = 0xff; //0b 1111 1111
                output[index++] = 0xff; //0b 1111 1111
              }
              output[index++] = distance >> 8;
              output[index++] = distance & 0xff;
            } else {
              output[index++] = (7 << 5) + 0x1f;
              for (len -= 7; len >= 0xff; len -= 0xff) output[index++] = 0xff;
              output[index++] = len;
              output[index++] = 0xff;
               for(int j=2;j<=i;j++){
                output[index++] = 0xff; //0b 1111 1111
                output[index++] = 0xff; //0b 1111 1111
              }
              output[index++] = distance >> 8;
              output[index++] = distance & 0xff;
            }
            break;
          }
      }
       return &output[index];
    }
      bool Matched_First_Every3Bytes(uint windows_size){
        sequence = read_uint32(input) & 0xffffff; //take every three bytes as a sequence  
        //(p[2] << 16) | (p[1] << 8) | p[0] from current input store consecutive 3 bytes.
        hash = HashTable::hash_func(sequence);//create the hash index
        ref = input_start + Hash.table[hash];  //input_start+ uint16
        Hash.table[hash] = input - input_start;//update the current index to hash table
        distance = input - ref; //offset
        input1=input;
        ref1=ref;
        cmp = (__builtin_expect(!!(distance < windows_size), 1))  ? read_uint32(ref)  
                  & 0xffffff : 0x1000000;   
            //check distance < WINDOW SIZE 
            //GNU Extension //likely
        return(sequence==cmp);
      }

      // The bytes before are matched, there exists a ref already ,so no need to update
      void Update_Hash(){
        input +=  match_len; //for the match boundary, the second byte of the last matched sequence 
        sequence = read_uint32(input);  // update hash table 
        hash = HashTable::hash_func(sequence & 0xffffff);
        Hash.table[hash] = input++ - input_start;//the third byte of the last matched sequence 
        sequence >>= 8; // & 0xffffff
        hash = HashTable::hash_func(sequence);
        Hash.table[hash] = input++ - input_start;//first of unwritten byte
      }

      void level1() {
        // literal copy
        while (input < input_limit)[[likely]] {
          // find potential match  
          while(input <= input_limit) [[likely]]{
            if (Matched_First_Every3Bytes(WINDOW_SIZE_L1)) break;   
            ++input;  //move on 1 byte to take a new sequence
          }   
          if (input >= input_limit) [[unlikely]]break;
          //write literal run part to the output             
          output = Literals_Output(input - pivot, pivot, output); 
          // copy(input-pivot+1) bytes from pivot to output

          //write short/long match part to the output
          match_len = match_cmp(ref + 3, input + 3, input_bound); 
          //ignore compared part and compare until input_bound (EOF)
          output = Match_Output1( match_len, distance, output);
          Update_Hash();
          pivot = input; // -> updated first of unwritten byte
        }// end of while 

        last_num_bytes = (ubyte*)INPUT_ + LENGTH_ - pivot;// the rest of unmatched part
        output = Literals_Output(last_num_bytes, pivot, output);//copy from pivot to EOF to output

        length_after= output - (ubyte*)OUTPUT_;
      }

      long long level2() {
        while (input < input_limit)[[likely]] {
          // potential match
          while(input <= input_limit)[[likely]]{
            if(Matched_First_Every3Bytes(65535 + MAX_L2_Length )) break;
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
          output = Literals_Output(input - pivot, pivot, output);

          match_len = match_cmp(ref + 3, input + 3, input_bound);
          output = Match_Output2( match_len, distance, output);
          
          Update_Hash();

          pivot = input;
        }

        last_num_bytes = (ubyte*)INPUT_ + LENGTH_ - pivot;
        output = Literals_Output (last_num_bytes, pivot, output);

        // id of this level
        *(ubyte*)OUTPUT_ |= (1 << 6); // default is all 0s

        length_after= output - (ubyte*)OUTPUT_;
         return length_after;
    }
    long long level3_old() {
        while (input < input_limit)[[likely]] {
          // potential match
          while(input <= input_limit)[[likely]]{
            if(Matched_First_Every3Bytes(65535 + 65535+MAX_L2_Length )) break;
            ++input;
          } // if not match to the previous,  continue to looping

          if (input >= input_limit) [[unlikely]]break;
            // if match to the previous one(>=3)

          /* far, needs at least 5-byte match */
          if (distance >= MAX_L2_Length) {
            if (ref[3] != input[3] || ref[4] != input[4]) {
              ++input;   //invalid 
              continue;  //continue finding the next matched
            }
          }
          /* far, needs at least 7-byte match */
          if (distance >= MAX_L2_Length+65535) {
            if (ref[3] != input[3] || ref[4] != input[4]||ref[5] != input[5]||ref[6] != input[6]) {
              ++input;   //invalid 
              continue;  //continue finding the next matched
            }
          }
          output = Literals_Output(input - pivot, pivot, output);

          match_len = match_cmp(ref + 3, input + 3, input_bound);
          // output = Match_Output3( match_len, distance, output);
          //
         
        {
        --distance;
        uint index=0;
        if (distance < MAX_L2_Length) {
          if (match_len < 7) {
            output[index++] = (match_len << 5) + (distance >> 8);
            output[index++] = (distance & 0xff);
          } else {
            output[index++] = (7 << 5) + (distance >> 8);
            for (match_len -= 7; match_len >= 0xff; match_len -= 0xff) output[index++] = 0xff;
            output[index++] = match_len;
            output[index++] = (distance & 0xff);
          }
        } else if(distance<MAX_L2_Length+65535){
          /* still far away... */
          if (match_len < 7) {//Level 2 Extended Windows* Short match
            distance -= MAX_L2_Length;
            output[index++] = (match_len << 5) + 0x1f;  
            output[index++] = 0xff;
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          } else {//Level 2 Extended Windows* Long match
            distance -= MAX_L2_Length;
            output[index++] = (7 << 5) + 0x1f;
            for (match_len -= 7; match_len >= 0xff; match_len -= 0xff) output[index++] = 0xff;
            output[index++] = match_len;
            output[index++] = 0xff;
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          }
        } else{ //level3
            if (match_len < 7) {//Level 3 Extended Windows* Short match
            distance -= MAX_L2_Length;
            distance -= 65535;
            output[index++] = (match_len << 5) + 0x1f;
            output[index++] = 0xff;
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          } else {//Level 3 Extended Windows* Long match


          //ref-(ubyte*)INPUT_; 

            distance -= MAX_L2_Length;
            distance -= 65535;
            output[index++] = (7 << 5) + 0x1f;
            for (match_len -= 7; match_len >= 0xff; match_len -= 0xff) {output[index++] = 0xff;}
            output[index++] = match_len;
            output[index++] = 0xff;
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = 0xff; //0b 1111 1111
            output[index++] = distance >> 8;
            output[index++] = distance & 0xff;
          }
        }
      output+=index;
      }
          //
          Update_Hash();

          pivot = input;
        }

        last_num_bytes = (ubyte*)INPUT_ + LENGTH_ - pivot;
        output = Literals_Output (last_num_bytes, pivot, output);

        // id of this level
        *(ubyte*)OUTPUT_ |= (1 << 7); // default is all 0s
        length_after= output - (ubyte*)OUTPUT_;
 
   
        return length_after;
    }
          long long level3() {
         while (input < input_limit)[[likely]] {
          // potential match
          while(input <= input_limit)[[likely]]{
            if(Matched_First_Every3Bytes(65535 +65535 + MAX_L2_Length )) break;
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
            /* far, needs at least 7-byte match */
          if (distance >= MAX_L2_Length+65535) {
            if (ref[3] != input[3] || ref[4] != input[4]||ref[5] != input[5]||ref[6] != input[6]) {
              ++input;   //invalid 
              continue;  //continue finding the next matched
            }
          }

          output = Literals_Output(input - pivot, pivot, output);

          match_len = match_cmp(ref + 3, input + 3, input_bound);
          output = Match_Output3( match_len, distance, output);
          
          Update_Hash();

          pivot = input;
        }

        last_num_bytes = (ubyte*)INPUT_ + LENGTH_ - pivot;
        output = Literals_Output (last_num_bytes, pivot, output);

        // id of this level
        *(ubyte*)OUTPUT_ |= (1 << 7); // default is all 0s

        length_after= output - (ubyte*)OUTPUT_;
        
 
 
         return length_after;
         
    }

      long long level4() {
        //  vector <int> count;
        int count=0;

        while (input < input_limit)[[likely]] {
          // potential match
          while(input <= input_limit)[[likely]]{
            if(Matched_First_Every3Bytes(input_limit-input )) break;
            ++input;
          } // if not match to the previous,  continue to looping

          if (input >= input_limit) [[unlikely]]break;
            // if match to the previous one(>=3)

          /* far, needs at least 5-byte match */
          // if (distance >= MAX_L2_Length) {
          //   if (ref[3] != input[3] || ref[4] != input[4]) {
          //     ++input;
          //     continue;
          //   }
          // }
          // if (distance >= MAX_L2_Length+65535) {
          //   if (ref[3] != input[3] || ref[4] != input[4]||ref[5] != input[5]||ref[6] != input[6]) {
          //     ++input;   //invalid 
          //     continue;  //continue finding the next matched
          //   }
          // }
          
          bool Invalid_Match=false;
          for(int i=0;i<=Window_Num;i++){
             /* far, needs at least 7-byte match */
            if (distance >= MAX_L2_Length+i*65535) {
              if (ref[3+i] != input[3+i] || ref[4+i] != input[4+i]) {
                Invalid_Match=true;    //invalid 
                break;  //continue finding the next matched
              }
            }
          }
          if(Invalid_Match){
            ++input;
            continue;
          }
          output = Literals_Output(input - pivot, pivot, output);

          match_len = match_cmp(ref + 3, input + 3, input_bound);

          output = Match_Output4( match_len, distance, output);
          
          Update_Hash();

          pivot = input;
        }

        last_num_bytes = (ubyte*)INPUT_ + LENGTH_ - pivot;
        output = Literals_Output (last_num_bytes, pivot, output);

        // id of this level
        *(ubyte*)OUTPUT_ |= (1 << 7); // default is all 0s

        length_after= output - (ubyte*)OUTPUT_;
        // for(auto i:count){
        //   cout<<i<<endl;
        // }
      
         return length_after;  
    }
};
