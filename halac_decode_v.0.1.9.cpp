#include <iostream>
#include "halac_decode_v.0.1.9.h"

void TRANSFORM(i32* data_L, i32* data_R, u32 sample_count, u8* OUT, u32& OUT_COUNTER) {

    i32 data_delta_L[sample_count];
    i32 data_delta_R[sample_count];

    data_delta_L[0] = data_L[0];
    data_delta_R[0] = data_R[0];

    i32 total = data_delta_L[0];
    for (u32 i = 1; i < sample_count; i++) {
        total += data_L[i];
        data_delta_L[i] = total;
    }

    total = data_delta_R[0];
    for (u32 i = 1; i < sample_count; i++) {
        total += data_R[i];
        data_delta_R[i] = total;
    }
    ///////////////////////////

    i32 data_transform_L[sample_count];
    i32 data_transform_R[sample_count];
    int X, Y;

    for (u32 i = 0; i < sample_count; i++) {
        X = data_delta_L[i];
        Y = data_delta_R[i];

        data_transform_R[i] = Y - (X / 2);
        data_transform_L[i] = X + data_transform_R[i];
    }
    ///////////////////////////

    i16 data[sample_count * CHANNEL_COUNT];
    for (u32 i = 0; i < sample_count; i++) {
        data[2 * i] = data_transform_L[i];
        data[2 * i + 1] = data_transform_R[i];
    }

    memcpy(OUT + OUT_COUNTER, data, sample_count * CHANNEL_COUNT * 2);
    OUT_COUNTER += sample_count * CHANNEL_COUNT * 2;
}

void UNCOMPRESS(u32 block_size, u8* OUT, u32& OUT_COUNTER) {
    ////////////////////////////
    if (OUT_COUNTER > 1024 * 512) {
        fwrite(OUT, 1, OUT_COUNTER, OUTPUT_FILE);
        OUT_COUNTER = 0;
    }
    ////////////////////////////

    float coeffs_L[ORDER] = { 0 };
    fread(coeffs_L, 4, ORDER, INPUT_FILE);
    ////////////////////////////////////

    u16 over_counter_L;
    fread(&over_counter_L, 1, 2, INPUT_FILE);
    u32 over_data_L[over_counter_L];
    if (over_counter_L > 0) fread(over_data_L, 4, over_counter_L, INPUT_FILE);
    //////////////////////////////////

    u32 fse_size_L_first;
    fread(&fse_size_L_first, 1, 4, INPUT_FILE);
    u8 fse_data_L_first[fse_size_L_first];
    u8 uncompressed_fse_data_L_first[block_size];

    if (fse_size_L_first > 1) {
        fread(fse_data_L_first, 1, fse_size_L_first, INPUT_FILE);
        FSE_decompress(uncompressed_fse_data_L_first, block_size, fse_data_L_first, fse_size_L_first);
    }
    else if (fse_size_L_first == 0) {
        fread(uncompressed_fse_data_L_first, 1, block_size, INPUT_FILE);
    }
    else if (fse_size_L_first == 1) {
        fread(fse_data_L_first, 1, 1, INPUT_FILE);
        fill(uncompressed_fse_data_L_first, uncompressed_fse_data_L_first + block_size, fse_data_L_first[0]);
    }
    //////////////////////////////////
    u32 huf_size_L_last;
    fread(&huf_size_L_last, 1, 4, INPUT_FILE);

    u8 huf_data_L_last[huf_size_L_last];
    u8 uncompressed_huf_data_L_last[block_size];

    if (huf_size_L_last > 1) {
        fread(huf_data_L_last, 1, huf_size_L_last, INPUT_FILE);
        HUF_decompress(uncompressed_huf_data_L_last, block_size, huf_data_L_last, huf_size_L_last);
    }
    else if (huf_size_L_last == 0) {
        fread(uncompressed_huf_data_L_last, 1, block_size, INPUT_FILE);
    }
    else if (huf_size_L_last == 1) {
        fread(huf_data_L_last, 1, 1, INPUT_FILE);
        fill(uncompressed_huf_data_L_last, uncompressed_huf_data_L_last + block_size, huf_data_L_last[0]);
    }

    //////////////////////////////////
    float coeffs_R[ORDER] = { 0 };
    fread(coeffs_R, 4, ORDER, INPUT_FILE);
    ////////////////////////////////////

    u16 over_counter_R;
    fread(&over_counter_R, 1, 2, INPUT_FILE);
    u32 over_data_R[over_counter_R];
    if (over_counter_R > 0) fread(over_data_R, 4, over_counter_R, INPUT_FILE);
    //////////////////////////////////

    u32 fse_size_R_first;
    fread(&fse_size_R_first, 1, 4, INPUT_FILE);

    u8 fse_data_R_first[fse_size_R_first];
    u8 uncompressed_fse_data_R_first[block_size];

    if (fse_size_R_first > 1) {
        fread(fse_data_R_first, 1, fse_size_R_first, INPUT_FILE);
        FSE_decompress(uncompressed_fse_data_R_first, block_size, fse_data_R_first, fse_size_R_first);
    }
    else if (fse_size_R_first == 0) {
        fread(uncompressed_fse_data_R_first, 1, block_size, INPUT_FILE);
    }
    else if (fse_size_R_first == 1) {
        fread(fse_data_R_first, 1, 1, INPUT_FILE);
        fill(uncompressed_fse_data_R_first, uncompressed_fse_data_R_first + block_size, fse_data_R_first[0]);
    }
    //////////////////////////////////
    u32 huf_size_R_last;
    fread(&huf_size_R_last, 1, 4, INPUT_FILE);

    u8 huf_data_R_last[huf_size_R_last];
    u8 uncompressed_huf_data_R_last[block_size];

    if (huf_size_R_last > 1) {
        fread(huf_data_R_last, 1, huf_size_R_last, INPUT_FILE);
        HUF_decompress(uncompressed_huf_data_R_last, block_size, huf_data_R_last, huf_size_R_last);
    }
    else if (huf_size_R_last == 0) {
        fread(uncompressed_huf_data_R_last, 1, block_size, INPUT_FILE);
    }
    else if (huf_size_R_last == 1) {
        fread(huf_data_R_last, 1, 1, INPUT_FILE);
        fill(uncompressed_huf_data_R_last, uncompressed_huf_data_R_last + block_size, huf_data_R_last[0]);
    }
    //////////////////////////////////

    u32 data_L;
    u32 data_R;
    i32 data_L_signed[block_size];
    i32 data_R_signed[block_size];
    u8 counter = 0;

    for (int i = 0; i < block_size; i++) {
        data_L = (uncompressed_fse_data_L_first[i] << 8) + uncompressed_huf_data_L_last[i];

        if (data_L == 65535) {
            data_L += over_data_L[counter];
            counter++;
        }

        data_L_signed[i] = Unsigned_To_Signed(data_L);
    }

    counter = 0;
    for (int i = 0; i < block_size; i++) {
        data_R = (uncompressed_fse_data_R_first[i] << 8) + uncompressed_huf_data_R_last[i];

        if (data_R == 65535) {
            data_R += over_data_R[counter];
            counter++;
        }

        data_R_signed[i] = Unsigned_To_Signed(data_R);
    }
    ///////////////////

    double coeffs_double_L[ORDER];
    double coeffs_double_R[ORDER];

    for (int i = 0; i < ORDER; i++) {
        coeffs_double_L[i] = coeffs_L[i];
        coeffs_double_R[i] = coeffs_R[i];
    }

    double predicted;
    for (int k = ORDER; k < block_size; k++) {
        predicted = 0.0;
        for (int j = 0; j < ORDER; j++) {
            predicted -= coeffs_double_L[j] * data_L_signed[k - 1 - j];
        }

        data_L_signed[k] = data_L_signed[k] + (int)predicted;
    }

    for (int k = ORDER; k < block_size; k++) {
        predicted = 0.0;
        for (int j = 0; j < ORDER; j++) {
            predicted -= coeffs_double_R[j] * data_R_signed[k - 1 - j];
        }

        data_R_signed[k] = data_R_signed[k] + (int)predicted;
    }

    TRANSFORM(data_L_signed, data_R_signed, block_size, OUT, OUT_COUNTER);
}

void DECODING() {
    READ_HALAC_HEADER();
    WRITE_WAV_HEADER();
    //////////////
    u8 OUT[1024 * 1024];
    u32 OUT_COUNTER = 0;

    u32 data_mod = INPUT_SIZE % BLOCK_SIZE;
    u32 block_count = INPUT_SIZE / BLOCK_SIZE - 1;
    for (int i = 0; i < block_count; i++) UNCOMPRESS(BLOCK_SIZE / (CHANNEL_COUNT * SAMPLE_BYTE), OUT, OUT_COUNTER);

    u32 last_block_size;
    if (block_count <= 0) last_block_size = INPUT_SIZE;
    else last_block_size = BLOCK_SIZE + data_mod;
    UNCOMPRESS(last_block_size / (CHANNEL_COUNT * SAMPLE_BYTE), OUT, OUT_COUNTER);

    if (OUT_COUNTER > 0) {
        fwrite(OUT, 1, OUT_COUNTER, OUTPUT_FILE);
    }
}

int main(int argc, char** argv)
{
    /*
     ///////////// Reading Console Arguments /////////////
     if (argc != 3) {
         printf(VERSION);
         printf(" Decoder\n"
             "Hakan Abbas (abbas.hakan@gmail.com)\n"
             "Only .halac types are supported for input!\n"
             "Please use this format: \"decoder.exe input_file output_file\"\n");
         return 0;
     }
     INPUT_FILENAME = argv[1];
     OUTPUT_FILENAME = argv[2];
     DECODING();
     /////////////////////////////////////////////////////
    */
    
    //////////////////////////////////////////
    clock_t startTime = clock();
    double time;

    string path;
    //path = "../WAV/ISKENDER_PAYDAS";
    //path = "../WAV/test";
    //path = "../WAV/TEST_AUDIO_SQUEEZE_CHART";
    //path = "../WAV/HANDE_YENER";
    //path = "../WAV/SIBEL_CAN";
    //path = "../WAV/TEST_AUDIOS";
    //path = "../WAV/YILDIZ_TILBE";
    //path = "../WAV/single";
    path = "D:/TEST_AUDIOS/YABANCI/Busta_Rhymes";
    //////////////////////////////////////////

    vector<string> files;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (!entry.is_directory() && (entry.path().extension() == ".halac"))
            files.emplace_back(entry.path().string());
    }
    sort(files.begin(), files.end());
    //////////////////////////////////////////

    TOTAL_OUTPUT_SIZE = 0;
    for (u32 i = 0; i < files.size(); i++) {
        INPUT_FILENAME = files.at(i);
        OUTPUT_FILENAME = INPUT_FILENAME + ".org";
        cout << OUTPUT_FILENAME << endl;

        OUTPUT_SIZE = 0;
        DECODING();
    }

    //////////////////////////////////////////
    time = static_cast<double>(clock() - (double)startTime) / CLOCKS_PER_SEC;
    printf("-----------\n");
    printf("Total Time: %2.3f seconds\n", time);
    //////////////////////////////////////////    
    
     return 0;
}

