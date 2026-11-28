#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include "FSE/fse.h"
#include "FSE/huf.h"
#include "XXH3/xxhash.h"
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
const i8 VERSION_FIRST[] = "HALAC 0.2.9";
const i8 VERSION_LAST[] = " Multithread Decoder(10 June 2024)";
const u32 FRAME_SIZE = 1024 * 512;

string INPUT_FILENAME, OUTPUT_FILENAME;
FILE *INPUT_FILE, *OUTPUT_FILE;
u64 FILE_SIZE, DATA_SIZE, OUTPUT_SIZE = 0, TOTAL_OUTPUT_SIZE = 0;
u32 SAMPLE_RATE, METADATA_FIRST_SIZE, METADATA_LAST_SIZE, FRAME_COUNT;
u16 CHANNEL_COUNT, SAMPLE_BIT, SAMPLE_BYTE;
bool OVERWRITE = 0;
u16 THREAD = 1;
u8 MODE = 0;
u32 LOSSYWAV_SIZE = 0;
//////////////////////////////////////////
//i8 FILE_TYPE[3];
i8 WAVECHUNK[6];
i8 BPSCHUNK[6];
i8 CHUNK_TYPE[4];
i8 METADATA_FIRST[1024 * 64];
i8 METADATA_LAST[1024 * 64];
i8 LOSSYWAV[1024 * 64];
//////////////////////////////////////////
u8* BUFFER[1024];
u8* OUT[1024];
u32 OUT_COUNTER[1024];
u32 COMPRESSED_FRAME_SIZES[1024 * 128];
//////////////////////////////////////////
void USAGE() {
    cout << VERSION_FIRST << VERSION_LAST << endl
        << "Hakan Abbas (abbas.hakan@gmail.com)" << endl
        << "Only .halac types are supported for input!" << endl << endl
        << "Please use in the following style;" << endl
        << "decoder input_file.halac output_file.wav [-OPTIONS]" << endl << endl
        << "[-OPTIONS]:" << endl
        << "-h: \"Help\"" << endl
        << "-ow: \"Overwrite the file\"" << endl
        << "-mt=[thread count]: \"Multithread mode (1 - 1024)\"" << endl << endl
        << "The default mode is \"Single Thread\"." << endl;
    exit(0);
}

static inline i32 U2S(u32 u) {
    return (u >> 1) ^ (-(u & 1)); // unsigned storage to signed
}

static inline u32 ABS(i32 n) {
    return (n + (n >> 31)) ^ (n >> 31); // 8 * 4 - 1 = 31 -> (sizeof(int) * CHARBIT - 1)
}

static inline int clampMax(int v, int max)
{
    //    return v > max ? max : v;
    return max + ((v - max) & ((v - max) >> 31));
}

static inline int clampMin(int v, int min)
{
    //    return v < min ? min : v;
    return min - ((min - v) & ((min - v) >> 31));
}

void FSE_DECODE(u8* uncompressed_data, u32 block_size, u8* BUFFER, u32& POS) {
    u16 fse_size;
    memcpy(&fse_size, BUFFER + POS, 2);
    POS += 2;

    u8 fse_data[fse_size];

    if (fse_size > 1) { // compress
        memcpy(fse_data, BUFFER + POS, fse_size);
        FSE_decompress(uncompressed_data, block_size, fse_data, fse_size);
        POS += fse_size;
    }
    else if (fse_size == 0) { // no compress
        memcpy(uncompressed_data, BUFFER + POS, block_size);
        POS += block_size;
    }
    else if (fse_size == 1) { // rle
        memcpy(fse_data, BUFFER + POS, 1);
        POS += 1;
        fill(uncompressed_data, uncompressed_data + block_size, fse_data[0]);
    }
}

void HUF_DECODE(u8* uncompressed_data, u32 block_size, u8* BUFFER, u32& POS) {
    u16 huf_size;
    memcpy(&huf_size, BUFFER + POS, 2);
    POS += 2;
    
    u8 huf_data[huf_size];

    if (huf_size > 0) { // compress
        memcpy(huf_data, BUFFER + POS, huf_size);
        HUF_decompress(uncompressed_data, block_size, huf_data, huf_size);
        POS += huf_size;
    }
    else if (huf_size == 0) { // no compress
        memcpy(uncompressed_data, BUFFER + POS, block_size);
        POS += block_size;
    }
}

void READ_HALAC_HEADER(string input_file_path) {
    INPUT_FILE = fopen(input_file_path.c_str(), "rb");
    if (!INPUT_FILE) {
        cout << "HALAC file could not read!" << endl;
        exit(0);
    }

    char version[11];
    fread(version, 1, 11, INPUT_FILE);

    if (version[0] != 'H' || version[1] != 'A' || version[2] != 'L' || version[3] != 'A' || version[4] != 'C') {
        cout << "Only HALAC file type is supported for input!" << endl;
        exit(0);
    }

    if (version[6] != '0' || version[8] != '2' || version[10] != '9') {
        cout << "Only " << VERSION_FIRST << " is supported for input!" << endl;
        exit(0);
    }

    fread(&MODE, 1, 1, INPUT_FILE);
    fseek(INPUT_FILE, 23, SEEK_CUR); // File Type and Time
    fread(&FILE_SIZE, 1, 8, INPUT_FILE); 
    fread(WAVECHUNK, 1, 6, INPUT_FILE);
    fread(&CHANNEL_COUNT, 1, 2, INPUT_FILE);
    if (CHANNEL_COUNT != 2) {
        cout << "Channel count must be 2!" << endl;
        exit(0);
    }

    fread(&SAMPLE_RATE, 1, 4, INPUT_FILE);
    fread(BPSCHUNK, 1, 6, INPUT_FILE);
    fread(&SAMPLE_BIT, 1, 2, INPUT_FILE);
    if (SAMPLE_BIT != 16) {
        cout << "Bits per sample must be 16!" << endl;
        exit(0);
    }

    SAMPLE_BYTE = SAMPLE_BIT / 8;

    fread(&DATA_SIZE, 1, 8, INPUT_FILE);
    /////////////////////////////////
    fread(&LOSSYWAV_SIZE, 1, 2, INPUT_FILE);
    fread(LOSSYWAV, 1, LOSSYWAV_SIZE, INPUT_FILE);
    
    /////////////////////////////////
    fread(&METADATA_FIRST_SIZE, 1, 2, INPUT_FILE); 
    fread(&METADATA_LAST_SIZE, 1, 2, INPUT_FILE);
    
    if (METADATA_FIRST_SIZE > 0) {
        fread(CHUNK_TYPE, 1, 4, INPUT_FILE);
        fread(METADATA_FIRST, 1, METADATA_FIRST_SIZE, INPUT_FILE);
    }

    fread(&FRAME_COUNT, 1, 4, INPUT_FILE);
    fread(COMPRESSED_FRAME_SIZES, 4, FRAME_COUNT, INPUT_FILE);

    //cout << FILE_SIZE << " " << CHANNEL_COUNT << " " << SAMPLE_RATE << " " << SAMPLE_BIT << " " 
    //     << SAMPLE_BYTE << " " << DATA_SIZE << " " << METADATA_FIRST_SIZE << " " << METADATA_FIRST << " " << FRAME_COUNT << endl;
}

void WRITE_WAV_HEADER(string output_file_path) {
    OUTPUT_FILE = fopen(output_file_path.c_str(), "wb");

    i8 RIFF[4] = { 'R','I','F','F' };
    i8 WAVEfmt[8] = { 'W','A','V','E','f','m','t',' '};
    i8 data[4] = { 'd', 'a', 't', 'a' };
    i8 fact[4] = { 'f', 'a', 'c', 't' };

    fwrite(RIFF, 1, 4, OUTPUT_FILE);
    fwrite(&FILE_SIZE, 1, 4, OUTPUT_FILE);
    fwrite(WAVEfmt, 1, 8, OUTPUT_FILE);
    fwrite(WAVECHUNK, 1, 6, OUTPUT_FILE);
    fwrite(&CHANNEL_COUNT, 1, 2, OUTPUT_FILE);
    fwrite(&SAMPLE_RATE, 1, 4, OUTPUT_FILE);
    fwrite(BPSCHUNK, 1, 6, OUTPUT_FILE);
    fwrite(&SAMPLE_BIT, 1, 2, OUTPUT_FILE);

    if (LOSSYWAV_SIZE > 0) {        
        fwrite(fact, 1, 4, OUTPUT_FILE);
        fwrite(&LOSSYWAV_SIZE, 1, 4, OUTPUT_FILE);
        fwrite(LOSSYWAV, 1, LOSSYWAV_SIZE, OUTPUT_FILE);
    }

    if (METADATA_FIRST_SIZE > 0) {
        fwrite(CHUNK_TYPE, 1, 4, OUTPUT_FILE);
        fwrite(&METADATA_FIRST_SIZE, 1, 4, OUTPUT_FILE);
        fwrite(METADATA_FIRST, 1, METADATA_FIRST_SIZE, OUTPUT_FILE);
    }

    fwrite(data, 1, 4, OUTPUT_FILE);
    fwrite(&DATA_SIZE, 1, 4, OUTPUT_FILE);
}