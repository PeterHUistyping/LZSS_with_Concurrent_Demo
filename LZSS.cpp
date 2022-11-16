#include<iostream>
using namespace std;
#include"LZSS_Compression.h"
#include"LZSS_Decompression.h"
long long numbytes;
// unsigned char* buffer = new unsigned char[numbytes];
// unsigned char* buffer2 = new unsigned char[numbytes];
// unsigned char* buffer3 = new unsigned char[numbytes];

void read_chunk_header(FILE* f,  long long* size,
                        long long* extra) {
  unsigned char buffer[8];
  fread(buffer, 1, 8, f);
  *size = read_uint32(buffer ) & 0xffffffff;
  *extra = read_uint32(buffer+4) & 0xffffffff;
}
void read_chunk_header(FILE* f,  long long* size) {
  unsigned char buffer[4];
  fread(buffer, 1, 4, f);
  *size = read_uint32(buffer) & 0xffffffff;
}
void write_chunk_header(FILE* f,  unsigned long long size,  unsigned long long extra) {
  unsigned char buffer[8];
  buffer[0] = size & 255;
  buffer[1] = (size >> 8) & 255;
  buffer[2] = (size >> 16) & 255;
  buffer[3] = (size >> 24) & 255;
 
  buffer[4] = extra & 255;
  buffer[5] = (extra >> 8) & 255;
  buffer[6] = (extra >> 16) & 255;
  buffer[7] = (extra >> 24) & 255;

  fwrite(buffer, 8, 1, f);
}
void write_chunk_header(FILE* f,  unsigned long long size) {
  unsigned char buffer[4];
  buffer[0] = size & 255;
  buffer[1] = (size >> 8) & 255;
  buffer[2] = (size >> 16) & 255;
  buffer[3] = (size >> 24) & 255;

  fwrite(buffer, 4, 1, f);
}
void LZSS_Comp_once(){
    FILE* infile =fopen("2CylinderEngine.obj","r");
      fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);
    //std::cout<<numbytes;
    // /* reset the file position indicator to 
    // the beginning of the file */
    fseek(infile, 0L, SEEK_SET);
    // read_chunk_header(infile,  &numbytes,  &chunk_extra);
    //unsigned char* buffer[num_threads];

      fclose(infile);   
        // int compress_level=2; //default using better
     unsigned char* buffer = new unsigned char[numbytes];
     unsigned char* buffer2 = new unsigned char[numbytes];
    fread( buffer , 1,numbytes,infile);
    // FILE *f=fopen("output_location","wb");
    // fwrite(buffer, 1, res.size(), f);
    // fclose(f);
    // long long chunk_size =fastlz_compress_level(compress_level, buffer
    //  , res.size(), buffer2);
    LZSS_Encoder Lzss_Encoder(buffer, numbytes, buffer2);
    //  long long chunk_size =Compress_LZ(buffer
    //  , res.size(), buffer2);
    long long chunk_size =Lzss_Encoder.Compress();
     FILE *f=fopen("output2.txt","wb");

     write_chunk_header(f, chunk_size,numbytes );
     //cout<<res.size()<<chunk_size<<endl;
     fwrite(buffer2, 1, chunk_size, f);
     fclose(f);   
    // static_cast<void*>(res.data()));
    // debug_writeout();
    // obj_output();

    delete [] buffer;
    delete [] buffer2;
}

void LZSS_Decom_once(){
     FILE* infile =fopen("output2.txt","rb");
    long long numbytes=0,chunk_extra=0;
    // /* Get the number of bytes */
    // // fseek(infile, 0L, SEEK_END);
    // // const long long numbytes = ftell(infile);
    
    // // /* reset the file position indicator to 
    // // the beginning of the file */
    // // fseek(infile, 0L, SEEK_SET);


    read_chunk_header(infile,  &numbytes,  &chunk_extra);
 
    unsigned char* buffer = new unsigned char[numbytes];
    unsigned char* buffer2 = new unsigned char[chunk_extra+10];
    fread(buffer, sizeof(char), numbytes, infile);
    // // FILE *f=fopen(output_location.c_str(),"wb");
    // // fwrite(buffer, 1, numbytes , f);
    // // fclose(f);
    LZSS_Decoder Lzss_Decoder(buffer, numbytes, buffer2);
    long long chunk_size =Lzss_Decoder.Decompress();
    // //cout<<numbytes<<chunk_extra<<chunk_size<<endl;
    // //FILE *f=fopen(output_location.c_str(),"wb");
    // //fwrite(buffer2, 1, chunk_extra , f);
    // //fclose(f);
   
    // //debug_readin();

    // /* for(long long i=0;i< chunk_size;i++){
    //     res.push_back(buffer2[i]);
    // } */
     FILE *f=fopen("Decompressed.txt","wb");
      fwrite(  buffer2 , 1,chunk_size , f);
     fclose(f);
    //Reconstruct before being deleted
    delete[]buffer;
    delete[]buffer2;
    fclose(infile);
}

int main(){
    LZSS_Comp_once(); 
    LZSS_Decom_once();
     return 0;

}