#include <iostream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <string.h>
#include "FSE/fse.h"
#include "FSE/huf.h"
//////////////////////////////////////////
using namespace std;
namespace fs = std::filesystem;
//////////////////////////////////////////
typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;
//////////////////////////////////////////
const i8 VERSION[] = "HALAC 0.1.9 - Single Thread";
string INPUT_FILENAME, OUTPUT_FILENAME;
FILE *INPUT_FILE, *OUTPUT_FILE;
u32 FILE_SIZE, INPUT_SIZE, DATA_SIZE, SAMPLE_RATE, METADATA_SIZE, OUTPUT_SIZE = 0, TOTAL_OUTPUT_SIZE = 0;
u16 CHANNEL_COUNT, SAMPLE_BIT, SAMPLE_BYTE;
const u32 BLOCK_SIZE = 1024 * 24;
const u8 ORDER = 8;

//////////////////////////////////////////
i8 TEMP1[14];
i8 TEMP2[6];
i8 METADATA[1024 * 16];
//////////////////////////////////////////
static inline i32 Unsigned_To_Signed(u32 num) {
    u32 mask = -(num & 1);
    return (num >> 1) ^ mask;
}

void READ_HALAC_HEADER() {
    INPUT_FILE = fopen(INPUT_FILENAME.c_str(), "rb");

    char version[11];
    fread(version, 1, 11, INPUT_FILE);

    if (version[0] != 'H' || version[1] != 'A' || version[2] != 'L' || version[3] != 'A' || version[4] != 'C') {
        printf("Only HALAC file type is supported for input!\n");
        exit(0);
    }

    fread(&FILE_SIZE, 1, 4, INPUT_FILE);
    fread(&TEMP1, 1, 14, INPUT_FILE);
    fread(&CHANNEL_COUNT, 1, 2, INPUT_FILE);
    fread(&SAMPLE_RATE, 1, 4, INPUT_FILE);
    fread(&TEMP2, 1, 6, INPUT_FILE);
    fread(&SAMPLE_BIT, 1, 2, INPUT_FILE);
    SAMPLE_BYTE = SAMPLE_BIT / 8;

    fread(&METADATA_SIZE, 1, 4, INPUT_FILE);
    if (METADATA_SIZE > 0) fread(&METADATA, 1, METADATA_SIZE, INPUT_FILE);

    fread(&INPUT_SIZE, 1, 4, INPUT_FILE);
}

void WRITE_WAV_HEADER() {
    OUTPUT_FILE = fopen(OUTPUT_FILENAME.c_str(), "wb");

    i8 RIFF[4] = { 'R','I','F','F' };
    i8 data[4] = { 'd', 'a', 't', 'a' };
    i8 LIST[4] = { 'L', 'I', 'S', 'T' };

    fwrite(RIFF, 1, 4, OUTPUT_FILE);
    fwrite(&FILE_SIZE, 1, 4, OUTPUT_FILE);
    fwrite(TEMP1, 1, 14, OUTPUT_FILE);
    fwrite(&CHANNEL_COUNT, 1, 2, OUTPUT_FILE);
    fwrite(&SAMPLE_RATE, 1, 4, OUTPUT_FILE);
    fwrite(TEMP2, 1, 6, OUTPUT_FILE);
    fwrite(&SAMPLE_BIT, 1, 2, OUTPUT_FILE);

    if (METADATA_SIZE > 0) {
        fwrite(LIST, 1, 4, OUTPUT_FILE);
        fwrite(&METADATA_SIZE, 1, 4, OUTPUT_FILE);
        fwrite(METADATA, 1, METADATA_SIZE, OUTPUT_FILE);
    }

    fwrite(data, 1, 4, OUTPUT_FILE);
    fwrite(&INPUT_SIZE, 1, 4, OUTPUT_FILE);
}