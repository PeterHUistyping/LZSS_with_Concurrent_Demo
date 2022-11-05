# Fast_LZ77 Understanding and Improving
A part of HUAWEI UK TECH Contest\ 
Implemented from https://github.com/ariya/FastLZ
## How to use this Demo
Input.txt -> Output.txt -> Decompressed.txt\
Open in Hex Editor (vscode extension)\
Ensure when decompressing, chunk_extra=numbytes;
## Update
Compress / Decompress\
Using namespace std;
## Fast_LZ77

### Compression Algorithm (Explained by a Demo)
```
Please open in text editor (txt format)
Reuse:  
Original Match-> Encoded
  1-2 Bytes   -> 1 Bytes (Literal Run)
  3-8 Bytes   -> 2 Bytes (Short Match)
  9-264 Bytes -> 3 Bytes (Long Match)

Original:
		------------
		I am Sam \n
		Sam I am \n
		That
		------------
Encoded:
--------------------------------------------------------------------------
		28 I am Sam \n 
   		20 03                00 20         	40 0C                  04 \nThat
--------------------------------------------------------------------------
	     |short match|        |literal run|         |short match|         |literal run|
High 3bits  Match length 001+2           1            Match length 010+2=4          5
                Offset 3           Copy space(20)         Offset 12             Copy \nThat
		  Sam                 [space]             I am                    \nThat


That Sam-I-am!
That Sam-I-am!
I do not like
that Sam-I-am!
Do you like green eggs and ham?
I do not like them, Sam-I-am.
I do not like green eggs and ham.
```

### Compression Src 
```
seq = flz_readu32(ip) & 0xffffff;
	seq=3Bytes staring from ip, for every ip from the third Byte
(As 1st,2nd Byte don't need to compress)


Build up a htab[HASH_SIZE: 1<<14];
	hash -> htab  ->ip offset from ip_start (3,4,5..)

ref = ip_start + htab[hash];
	refer to the first time seq (3Bytes staring from ip) appears in htab

distance = ip - ref;
	LZ77 offset

cmp = FASTLZ_LIKELY(distance < MAX_FARDISTANCE) ? flz_readu32(ref) & 0xffffff : 0x1000000;
	comparing 3Bytes staring from ip and 3Bytes staring from ref
	if cmp==seq -> break;

const uint8_t* anchor = ip;
	start from the first that haven't been written, updated every written


	Main function:
flz_literals(ip - anchor, anchor, op);
	write from anchor to ip into op 

flz1_match(len, distance, op);
	write match length and offset(distance here) into op
```