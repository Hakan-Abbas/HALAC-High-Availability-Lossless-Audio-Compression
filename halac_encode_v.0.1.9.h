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
FILE* INPUT_FILE, * OUTPUT_FILE;
u32 FILE_SIZE, INPUT_SIZE, DATA_SIZE, SAMPLE_RATE, METADATA_SIZE, OUTPUT_SIZE = 0, TOTAL_OUTPUT_SIZE = 0;
u16 CHANNEL_COUNT, SAMPLE_BIT, SAMPLE_BYTE;
const u32 BLOCK_SIZE = 1024 * 24;
const u8 ORDER = 8;
//////////////////////////////////////////
i8 FILE_TYPE[4];
i8 TEMP1[14];
i8 TEMP2[6];
i8 METADATA[1024 * 16];
//////////////////////////////////////////
static inline u32 Signed_To_Unsigned(i32 num) {
    u32 mask = num >> 31;
    return (num << 1) ^ mask;
}

u32 HUF_ENCODE(u8* data, u32 data_size, u8* OUT, u32 &OUT_COUNTER) {
    const u32 max_compressed_size = HUF_compressBound(data_size);
    u8 compressed[max_compressed_size];
    u32 compressed_size = HUF_compress(compressed, max_compressed_size, data, data_size);

    memcpy(OUT + OUT_COUNTER, &compressed_size, 4); OUT_COUNTER += 4;

    if (compressed_size == 0) {     
        memcpy(OUT + OUT_COUNTER, data, data_size); OUT_COUNTER += data_size;
        compressed_size = data_size;
    }
    else {
        memcpy(OUT + OUT_COUNTER, compressed, compressed_size); OUT_COUNTER += compressed_size;
    }

    OUTPUT_SIZE += compressed_size;
    return compressed_size;
}

u32 FSE_ENCODE(u8* data, u32 data_size, u8* OUT, u32 &OUT_COUNTER) {
    const u32 max_compressed_size = FSE_compressBound(data_size);
    u8 compressed[max_compressed_size];
    u32 compressed_size = FSE_compress(compressed, max_compressed_size, data, data_size);

    memcpy(OUT + OUT_COUNTER, &compressed_size, 4); OUT_COUNTER += 4;

    if (compressed_size > 1) {
        memcpy(OUT + OUT_COUNTER, compressed, compressed_size); OUT_COUNTER += compressed_size;
    }
    else if (compressed_size == 1) {
        memcpy(OUT + OUT_COUNTER, &data[0], 1); OUT_COUNTER += 1;
        compressed_size++;
    }
    else {
        memcpy(OUT + OUT_COUNTER, data, data_size); OUT_COUNTER += data_size;
        compressed_size = data_size;
    }

    OUTPUT_SIZE += compressed_size;
    return compressed_size;
}

void READ_WAV_HEADER() {
    INPUT_FILE = fopen(INPUT_FILENAME.c_str(), "rb");

    char RIFF[4];
    fread(RIFF, 1, 4, INPUT_FILE);

    if (RIFF[0] != 'R' || RIFF[1] != 'I' || RIFF[2] != 'F' || RIFF[3] != 'F') {
        printf("Only WAV file type is supported for input!\n");
        exit(0);
    }

    fread(&FILE_SIZE, 1, 4, INPUT_FILE);
    if (FILE_SIZE < 1024) {
        printf("Minimum file size must be largen than 1kb!\n");
        return;
    }

    fread(&TEMP1, 1, 14, INPUT_FILE);
    fread(&CHANNEL_COUNT, 1, 2, INPUT_FILE);
    fread(&SAMPLE_RATE, 1, 4, INPUT_FILE);
    fread(&TEMP2, 1, 6, INPUT_FILE);
    fread(&SAMPLE_BIT, 1, 2, INPUT_FILE);
    SAMPLE_BYTE = SAMPLE_BIT / 8;
    METADATA_SIZE = 0;

    //////////////////
    fread(FILE_TYPE, 1, 4, INPUT_FILE);

    if (FILE_TYPE[0] == 'd' && FILE_TYPE[1] == 'a' && FILE_TYPE[2] == 't' && FILE_TYPE[3] == 'a') {
        fread(&INPUT_SIZE, 1, 4, INPUT_FILE);
    }
    else {
        fread(&METADATA_SIZE, 1, 4, INPUT_FILE);
        fread(METADATA, 1, METADATA_SIZE, INPUT_FILE);

        fseek(INPUT_FILE, 4, SEEK_CUR);
        fread(&INPUT_SIZE, 1, 4, INPUT_FILE);
    }
}

void WRITE_HALAC_HEADER() {
    OUTPUT_FILE = fopen(OUTPUT_FILENAME.c_str(), "wb");

    fwrite(VERSION, 1, 11, OUTPUT_FILE);
    fwrite(&FILE_SIZE, 1, 4, OUTPUT_FILE);
    fwrite(&TEMP1, 1, 14, OUTPUT_FILE);
    fwrite(&CHANNEL_COUNT, 1, 2, OUTPUT_FILE);
    fwrite(&SAMPLE_RATE, 1, 4, OUTPUT_FILE);
    fwrite(&TEMP2, 1, 6, OUTPUT_FILE);
    fwrite(&SAMPLE_BIT, 1, 2, OUTPUT_FILE);

    fwrite(&METADATA_SIZE, 1, 4, OUTPUT_FILE);
    if (METADATA_SIZE > 0) {
        fwrite(METADATA, 1, METADATA_SIZE, OUTPUT_FILE);
    }

    fwrite(&INPUT_SIZE, 1, 4, OUTPUT_FILE);
}