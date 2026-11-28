#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include "FSE/fse.h"
#include "FSE/huf.h"
//#include "RANS/rANS_static4x16.h"
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
const i8 VERSION[] = "HALAC 0.2.9 Multithread Encoder (10 June 2024)";
const u32 FRAME_SIZE = 1024 * 512;

string INPUT_FILENAME, OUTPUT_FILENAME;
FILE* INPUT_FILE, * OUTPUT_FILE;
u64 FILE_SIZE, DATA_SIZE, OUTPUT_SIZE = 0, TOTAL_OUTPUT_SIZE = 0;
u32 SAMPLE_RATE, METADATA_FIRST_SIZE, METADATA_LAST_SIZE;
u16 CHANNEL_COUNT, SAMPLE_BIT, SAMPLE_BYTE;
bool OVERWRITE = 0;
u16 THREAD = 1;
u8 MODE = 0;
u16 LOSSYWAV_SIZE = 0;
//////////////////////////////////////////
i8 FILE_TYPE[3];
i8 CHUNK_TYPE[4];
i8 WAVECHUNK[6];
i8 BPSCHUNK[6];
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
    cout << VERSION << endl
        << "Hakan Abbas (abbas.hakan@gmail.com)" << endl
        << "Only 16 bit, 2 channels .wav types are supported for input!" << endl << endl
        << "Please use in the following style;" << endl
        << "encoder input_file.wav output_file.halac [-OPTIONS]" << endl << endl
        << "[-OPTIONS]:" << endl
        << "-h: \"Help\"" << endl
        << "-ow: \"Overwrite the file\"" << endl
        << "-fast: \"Fast mode\"" << endl
        << "-mt=[thread count]: \"Multithread mode (1 - 1024)\"" << endl << endl
        << "The default modes are \"Normal\" and \"Single Thread\"." << endl;
    exit(0);
}

u32 BIT_COUNT(u32 x) {
    u32 result = 0;
    while (x >>= 1) ++result;
    return result + 1;
}

static inline double fast_log2(double val) {
    union { double d; uint64_t x; } u = { val };
    const int E = ((u.x >> 52) & 2047) - 1024; // exponent E
    // Initial log2(M) based on mantissa
    u.x &= ~(2047LL << 52);
    u.x += 1023LL << 52;
    val = ((-1 / 3.) * u.d + 2) * u.d - 2 / 3.;
    return E + val;
}

double ENTROPY(u8* data, u32 data_size) {
    ////////////////////////////
    // Calculating Histogram
    int F0[256] = { 0 }, F1[256] = { 0 }, F2[256] = { 0 }, F3[256] = { 0 };
    int i, i4 = data_size & ~3;
    for (i = 0; i < i4;) {
        F0[data[i++]]++;
        F1[data[i++]]++;
        F2[data[i++]]++;
        F3[data[i++]]++;
    }
    while (i < data_size)
        F0[data[i++]]++;

    for (i = 0; i < 256; i++)
        F0[i] += F1[i] + F2[i] + F3[i];

    ////////////////////////////
    // Calculating Entropy
    double entropy = 0;
    for (int i = 0; i < 256; i++) {
        double x = F0[i] / (double)data_size;
        entropy += x * -fast_log2(x);
    }
    ////////////////////////////
    return entropy;
}

static inline u32 S2U(i32 s) {
    //    return s > 0 ? (s << 1) : (-s << 1) - 1;
    return (s << 1) ^ (s >> 31); // signed to unsigned storage
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

char* WRITE_TIME() {
    time_t current_time = time(NULL);
    struct tm* local_time_info = localtime(&current_time);
    char time_string[20] = { 0 };
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time_info);
    //cout << time_string << endl;
    return time_string;
}

//u32 RANS_ENCODE(u8* data, u32 data_size, u32 FRAME_INDEX, u8 order) {
//    u32 compressed_size;
//    u8* compressed = rans_compress_4x16(data, data_size, &compressed_size, order);
//
//    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &compressed_size, 4); OUT_COUNTER[FRAME_INDEX] += 4;
//
//    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], compressed, compressed_size); OUT_COUNTER[FRAME_INDEX] += compressed_size;
//
//    free(compressed);
//    OUTPUT_SIZE += compressed_size;
//
//    return compressed_size;
//}

u32 HUF_ENCODE(u8* data, u32 data_size, u32 FRAME_INDEX) {
    const u16 max_compressed_size = HUF_compressBound(data_size);
    u8 compressed[max_compressed_size];
    u16 compressed_size = HUF_compress(compressed, max_compressed_size, data, data_size);

    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &compressed_size, 2); OUT_COUNTER[FRAME_INDEX] += 2;

    if (compressed_size == 0) { //cout << "No Compress"<< endl;        
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], data, data_size); OUT_COUNTER[FRAME_INDEX] += data_size;
        compressed_size = data_size;
    }
    else {
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], compressed, compressed_size); OUT_COUNTER[FRAME_INDEX] += compressed_size;
    }

    OUTPUT_SIZE += compressed_size;
    return compressed_size;
}

u32 FSE_ENCODE(u8* data, u32 data_size, u32 FRAME_INDEX) {
    const u16 max_compressed_size = FSE_compressBound(data_size);
    u8 compressed[max_compressed_size];
    u16 compressed_size = FSE_compress(compressed, max_compressed_size, data, data_size);

    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &compressed_size, 2); OUT_COUNTER[FRAME_INDEX] += 2;

    if (compressed_size > 1) { // cout << "FSE"<< endl;
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], compressed, compressed_size); OUT_COUNTER[FRAME_INDEX] += compressed_size;
    }
    else if (compressed_size == 1) { // cout << "RLE"<< endl;
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &data[0], 1); OUT_COUNTER[FRAME_INDEX] += 1;
        compressed_size++;
    }
    else { //cout << "No Compress"<< endl;
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], data, data_size); OUT_COUNTER[FRAME_INDEX] += data_size;
        compressed_size = data_size;
    }

    OUTPUT_SIZE += compressed_size;
    return compressed_size;
}

void READ_WAV_HEADER(string input_file_path) {
    INPUT_FILE = fopen(input_file_path.c_str(), "rb");
    if (!INPUT_FILE) {
        cout << "WAV file could not read!" << endl;
        exit(0);
    }

    char RIFF[4];
    fread(RIFF, 1, 4, INPUT_FILE);
    if (RIFF[0] != 'R' || RIFF[1] != 'I' || RIFF[2] != 'F' || RIFF[3] != 'F') {
        cout << "Only WAV file type is supported for input!" << endl;
        exit(0);
    }

    char file_type[] = "wav";
    memcpy(FILE_TYPE, file_type, 3);

    fread(&FILE_SIZE, 1, 4, INPUT_FILE);
    if (FILE_SIZE < 1024) {
        cout << "Minimum file size must be largen than 1kb!" << endl;
        exit(0);
    }

    fseek(INPUT_FILE, 8, SEEK_CUR); // wavefmt
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

    //////////////////
    fread(CHUNK_TYPE, 1, 4, INPUT_FILE);
    
    LOSSYWAV_SIZE = 0;
    if (CHUNK_TYPE[0] == 'f' && CHUNK_TYPE[1] == 'a' && CHUNK_TYPE[2] == 'c' && CHUNK_TYPE[3] == 't') { // lossyWAV
        fread(&LOSSYWAV_SIZE, 1, 4, INPUT_FILE);
        fread(LOSSYWAV, 1, LOSSYWAV_SIZE, INPUT_FILE);
        MODE = 2;

        fread(CHUNK_TYPE, 1, 4, INPUT_FILE);
    }

    if (CHUNK_TYPE[0] == 'd' && CHUNK_TYPE[1] == 'a' && CHUNK_TYPE[2] == 't' && CHUNK_TYPE[3] == 'a') { // data
        METADATA_FIRST_SIZE = 0;
        fread(&DATA_SIZE, 1, 4, INPUT_FILE);
    }
    else { // LIST - JUNK vs.
        fread(&METADATA_FIRST_SIZE, 1, 4, INPUT_FILE);         
        fread(METADATA_FIRST, 1, METADATA_FIRST_SIZE, INPUT_FILE);

        fseek(INPUT_FILE, 4, SEEK_CUR); // data
        fread(&DATA_SIZE, 1, 4, INPUT_FILE);
    }

    //cout << (int)MODE << endl;
    //cout << METADATA_FIRST_SIZE << " " << METADATA_LAST_SIZE << endl;
    //cout << FILE_SIZE << " " << DATA_SIZE + 44 << " " << CHANNEL_COUNT << " " << SAMPLE_RATE << " " << SAMPLE_BIT << endl;
}

void WRITE_HALAC_HEADER(string output_file_path) {
    OUTPUT_FILE = fopen(output_file_path.c_str(), "wb");

    fwrite(VERSION, 1, 11, OUTPUT_FILE);
    fwrite(&MODE, 1, 1, OUTPUT_FILE);
    fwrite(FILE_TYPE, 1, 3, OUTPUT_FILE);
    fwrite(WRITE_TIME(), 1, 20, OUTPUT_FILE);

    fwrite(&FILE_SIZE, 1, 8, OUTPUT_FILE);
    fwrite(WAVECHUNK, 1, 6, OUTPUT_FILE);
    fwrite(&CHANNEL_COUNT, 1, 2, OUTPUT_FILE);
    fwrite(&SAMPLE_RATE, 1, 4, OUTPUT_FILE);
    fwrite(BPSCHUNK, 1, 6, OUTPUT_FILE);
    fwrite(&SAMPLE_BIT, 1, 2, OUTPUT_FILE);
    fwrite(&DATA_SIZE, 1, 8, OUTPUT_FILE);

    fwrite(&LOSSYWAV_SIZE, 1, 2, OUTPUT_FILE);
    if (LOSSYWAV_SIZE > 0) {
        fwrite(LOSSYWAV, 1, LOSSYWAV_SIZE, OUTPUT_FILE);
    }

    fwrite(&METADATA_FIRST_SIZE, 1, 2, OUTPUT_FILE);

    METADATA_LAST_SIZE = 0;
    fwrite(&METADATA_LAST_SIZE, 1, 2, OUTPUT_FILE);
 
    if (METADATA_FIRST_SIZE > 0) {
        fwrite(CHUNK_TYPE, 1, 4, OUTPUT_FILE);
        fwrite(METADATA_FIRST, 1, METADATA_FIRST_SIZE, OUTPUT_FILE);
        METADATA_FIRST_SIZE += 4; // LIST - JUNK
    }
    
}