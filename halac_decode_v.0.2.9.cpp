#include "halac_decode_v.0.2.9.h"

void TRANSFORM(i32* data_L, i32* data_R, u32 sample_count, u8 transform_type_L, u8 transform_type_R, u32 FRAME_INDEX) {

    if (transform_type_L == 0) {
        for (u32 i = 1; i < sample_count; i++) data_L[i] += data_L[i - 1];
    }
    else {
        for (u32 i = 2; i < sample_count; i++) {
            i32 P = 2 * data_L[i - 1] - data_L[i - 2];
            data_L[i] += P;
        }
    }
    ///////////////////////////
    if (transform_type_R == 0) {
        for (u32 i = 1; i < sample_count; i++) data_R[i] += data_R[i - 1];
    }
    else {
        for (u32 i = 2; i < sample_count; i++) {
            i32 P = 2 * data_R[i - 1] - data_R[i - 2];
            data_R[i] += P;
        }
    }

    ///////////////////////////    
    if (MODE != 2) {
        int X, Y;
        for (u32 i = 0; i < sample_count; i++) {
            X = data_L[i];
            Y = data_R[i];

            data_R[i] = Y - (X / 2);
            data_L[i] = X + data_R[i];
        }
    }

    ///////////////////////////
    i16 data[sample_count * CHANNEL_COUNT];
    for (u32 i = 0; i < sample_count; i++) {
        data[2 * i] = data_L[i];
        data[2 * i + 1] = data_R[i];
    }

    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], data, sample_count * CHANNEL_COUNT * 2); // 16 bit
    OUT_COUNTER[FRAME_INDEX] += sample_count * CHANNEL_COUNT * 2;
}

void UNCOMPRESS(u8* BUFFER, u32 block_size, u32 FRAME_INDEX, u32 &POS) {
    u8 ORDER;     
    if (MODE == 0) ORDER = 8;
    else if (MODE == 1) ORDER = 2;
    else if (MODE == 2) ORDER = 0;
 
    //////////////////////////////
    //float16 coeffs_L[32] = { 0 };
    u8 coeffs_index_L[32] = { 0 };
    memcpy(coeffs_index_L, BUFFER + POS, ORDER); POS += ORDER;
    ////////////////////////////
    u8 side_info;
    memcpy(&side_info, BUFFER + POS, 1); POS += 1;

    u8 transform_type_L = side_info & 0x01; // 1 bit
    u8 max_bit_count = (side_info >> 1) & 0x1F; // 5 bit
    ////////////////////////////

    u8 uncompressed_data_L_first[block_size];
    u8 uncompressed_data_L_last[block_size];
    u32 over_data_L[1024 * 16];
    u32 data_L;
    i32 data_L_signed[block_size];
    ////////////////////////////////////

    if (max_bit_count <= 8) {
        if (MODE != 1) FSE_DECODE(uncompressed_data_L_last, block_size, BUFFER, POS);
        else if (MODE == 1) HUF_DECODE(uncompressed_data_L_last, block_size, BUFFER, POS);

        for (int i = 0; i < block_size; i++) {
            data_L = uncompressed_data_L_last[i];
            data_L_signed[i] = U2S(data_L);
        }
    }
    else if (max_bit_count <= 16) {
        if (MODE != 1) FSE_DECODE(uncompressed_data_L_first, block_size, BUFFER, POS);
        else if (MODE == 1) HUF_DECODE(uncompressed_data_L_first, block_size, BUFFER, POS);

        u8 fse_state = (side_info >> 6) & 0x01; // 1 bit
        if (fse_state) {
            if (MODE != 1) FSE_DECODE(uncompressed_data_L_last, block_size, BUFFER, POS);
            else if (MODE == 1) HUF_DECODE(uncompressed_data_L_last, block_size, BUFFER, POS);
        }
        else HUF_DECODE(uncompressed_data_L_last, block_size, BUFFER, POS);
        //////////////////////////////////

        for (int i = 0; i < block_size; i++) {
            data_L = (uncompressed_data_L_first[i] << 8) + uncompressed_data_L_last[i];
            data_L_signed[i] = U2S(data_L);
        }
    }
    else {
        u16 over_counter_L;
        memcpy(&over_counter_L, BUFFER + POS, 2); POS += 2;

        if (over_counter_L > 0) {
            memcpy(over_data_L, BUFFER + POS, over_counter_L * 4);
            POS += over_counter_L * 4;
        }

        if (MODE != 1) FSE_DECODE(uncompressed_data_L_first, block_size, BUFFER, POS);
        else if (MODE == 1) HUF_DECODE(uncompressed_data_L_first, block_size, BUFFER, POS);
        
        HUF_DECODE(uncompressed_data_L_last, block_size, BUFFER, POS);
        //////////////////////////////////

        u16 counter = 0;
        for (int i = 0; i < block_size; i++) {
            data_L = (uncompressed_data_L_first[i] << 8) + uncompressed_data_L_last[i];

            if (data_L == 65535) {
                data_L += over_data_L[counter];
                counter++;
            }
            data_L_signed[i] = U2S(data_L);
        }
    }
    //////////////////////////////////

    //float16 coeffs_R[32] = { 0 };
    u8 coeffs_index_R[32] = { 0 };
    memcpy(coeffs_index_R, BUFFER + POS, ORDER); POS += ORDER;
    ////////////////////////////
    memcpy(&side_info, BUFFER + POS, 1); POS += 1;

    u8 transform_type_R = side_info & 0x01; // 1 bit
    max_bit_count = (side_info >> 1) & 0x1F; // 5 bit
    
    ////////////////////////////////////
    u8 uncompressed_data_R_first[block_size];
    u8 uncompressed_data_R_last[block_size];
    u32 over_data_R[1024 * 16];
    u32 data_R;
    i32 data_R_signed[block_size];
    ////////////////////////////////////

    if (max_bit_count <= 8) {        
        if (MODE != 1) FSE_DECODE(uncompressed_data_R_last, block_size, BUFFER, POS);
        else if (MODE == 1) HUF_DECODE(uncompressed_data_R_last, block_size, BUFFER, POS);
                
        for (int i = 0; i < block_size; i++) {
            data_R = uncompressed_data_R_last[i];
            data_R_signed[i] = U2S(data_R);
        }
    }
    else if (max_bit_count <= 16) {        
        if (MODE != 1) FSE_DECODE(uncompressed_data_R_first, block_size, BUFFER, POS);
        else if (MODE == 1) HUF_DECODE(uncompressed_data_R_first, block_size, BUFFER, POS);

        u8 fse_state = (side_info >> 6) & 0x01; // 1 bit
        if (fse_state) {
            if (MODE != 1) FSE_DECODE(uncompressed_data_R_last, block_size, BUFFER, POS);
            else if (MODE == 1) HUF_DECODE(uncompressed_data_R_last, block_size, BUFFER, POS);
        }
        else HUF_DECODE(uncompressed_data_R_last, block_size, BUFFER, POS);
        //////////////////////////////////

        for (int i = 0; i < block_size; i++) {
            data_R = (uncompressed_data_R_first[i] << 8) + uncompressed_data_R_last[i];
            data_R_signed[i] = U2S(data_R);
        }
    }
    else {
        u16 over_counter_R;
        memcpy(&over_counter_R, BUFFER + POS, 2); POS += 2;

        if (over_counter_R > 0) {
            memcpy(over_data_R, BUFFER + POS, over_counter_R * 4);
            POS += over_counter_R * 4;
        }

        if (MODE != 1) FSE_DECODE(uncompressed_data_R_first, block_size, BUFFER, POS);
        else if (MODE == 1) HUF_DECODE(uncompressed_data_R_first, block_size, BUFFER, POS);

        HUF_DECODE(uncompressed_data_R_last, block_size, BUFFER, POS);
        //////////////////////////////////
        u16 counter = 0;
        for (int i = 0; i < block_size; i++) {
            data_R = (uncompressed_data_R_first[i] << 8) + uncompressed_data_R_last[i];

            if (data_R == 65535) {
                data_R += over_data_R[counter];
                counter++;
            }
            data_R_signed[i] = U2S(data_R);
        }
    }
    
    //////////////////////////////////
    double coeffs_double_L[ORDER];
    double coeffs_double_R[ORDER];

    for (int i = 0; i < ORDER; i++) {
        coeffs_double_L[i] = (- 2.56 + coeffs_index_L[i] * 0.02);
    }
    //cout << endl;

    for (int i = 0; i < ORDER; i++) {
        coeffs_double_R[i] = (- 2.56 + coeffs_index_R[i] * 0.02);
    }
    //cout << endl;
    ///////////////////

    double predicted;

    if (MODE == 0) {
        const u8 order = 8;
        for (int k = order; k < block_size; k++) {
            predicted = 0.0;
            for (int j = 0; j < order; j++) {
                predicted -= (coeffs_double_L[j] * 1.00001) * data_L_signed[k - 1 - j];
                //cout << k << " " << coeffs_double_L[j] << " " << data_L_signed[k - 1 - j] << " " << predicted << endl;
            }

            predicted = clampMax(predicted, 65535);
            predicted = clampMin(predicted, -65536);

            data_L_signed[k] = data_L_signed[k] + predicted;
        }

        for (int k = order; k < block_size; k++) {
            predicted = 0.0;
            for (int j = 0; j < order; j++) {
                predicted -= (coeffs_double_R[j] * 1.00001) * data_R_signed[k - 1 - j];
            }

            predicted = clampMax(predicted, 65535);
            predicted = clampMin(predicted, -65536);

            data_R_signed[k] = data_R_signed[k] + predicted;
        }
    }
    else if (MODE == 1) {
        const u8 order = 2;
        for (int k = order; k < block_size; k++) {
            predicted = 0.0;
            for (int j = 0; j < order; j++) {
                predicted -= (coeffs_double_L[j] * 1.00001) * data_L_signed[k - 1 - j];
            }

            predicted = clampMax(predicted, 65535);
            predicted = clampMin(predicted, -65536);

            data_L_signed[k] = data_L_signed[k] + predicted;
        }

        for (int k = order; k < block_size; k++) {
            predicted = 0.0;
            for (int j = 0; j < order; j++) {
                predicted -= (coeffs_double_R[j] * 1.00001) * data_R_signed[k - 1 - j];
            }

            predicted = clampMax(predicted, 65535);
            predicted = clampMin(predicted, -65536);

            data_R_signed[k] = data_R_signed[k] + predicted;
        }
    }
    else if (MODE == 2) {
        // NO PROCESS
    }
    TRANSFORM(data_L_signed, data_R_signed, block_size, transform_type_L, transform_type_R, FRAME_INDEX);
}

void DECODING(u8* BUFFER, u32 FRAME_SIZE, u32 FRAME_INDEX, u32 frame_counter) {
    u32 BLOCK_SIZE;
    if (MODE == 0) BLOCK_SIZE = 1024 * 16; // MODE 0 -> "-normal"
    else if (MODE == 1) BLOCK_SIZE = 1024 * 32; // MODE 1 -> "-fast"
    else if (MODE == 2) BLOCK_SIZE = 1024 * 8; // MODE 2 -> "-lossywav"

    //////////////

    u32 division = FRAME_SIZE / BLOCK_SIZE;
    u32 mod = FRAME_SIZE % BLOCK_SIZE;
    u32 block_count;
    u32 block_sizes[division + 1];

    if (division == 0) {
        block_count = 1;
        block_sizes[0] = mod;
    }
    else {
        block_count = division;
        for (int i = 0; i < block_count; i++) block_sizes[i] = BLOCK_SIZE;
        if (mod > BLOCK_SIZE / 2) {
            block_sizes[block_count] = mod;
            block_count++;
        }
        else {
            block_sizes[block_count - 1] = BLOCK_SIZE + mod;
        }
    }
    ////////////////////////////
    
    u32 POS = 15;
    //////////////////////////////////////////
    //i8 VERSION[3];
    //memcpy(VERSION, BUFFER, 3); POS += 3;
    //memcpy(&MODE, BUFFER + POS, 1); POS += 1;
    //memcpy(&CHANNEL_COUNT, BUFFER + POS, 1); POS += 1;
    //memcpy(&SAMPLE_BIT, BUFFER + POS, 1); POS += 1;
    //memcpy(&SAMPLE_RATE, BUFFER + POS, 3); POS += 3;
    //memcpy(&FRAME_SIZE, BUFFER + POS, 3); POS += 3;
    //u32 COMPRESSED_FRAME_SIZE;
    //memcpy(&COMPRESSED_FRAME_SIZE, BUFFER + POS, 3); POS += 3;

    u32 HASH_CONTROL;
    memcpy(&HASH_CONTROL, BUFFER + POS, 4); POS += 4;
    //////////////////////////////////////////

    for (int i = 0; i < block_count; i++) { // block_count
        UNCOMPRESS(BUFFER, block_sizes[i] / (CHANNEL_COUNT * SAMPLE_BYTE), FRAME_INDEX, POS);
    }

    //////////////////////////////////////////
    u32 HASH = (u32)XXH3_64bits(OUT[FRAME_INDEX], OUT_COUNTER[FRAME_INDEX]); //cout << frame_counter << endl;
    if (HASH != HASH_CONTROL) {
        cout << "The decoded file has a corruption in the frame " << frame_counter << "! (around byte " << FRAME_SIZE * frame_counter << ")" << endl;
    }
}

void FRAMING() {
    u32 mod = DATA_SIZE % FRAME_SIZE;
    u32 frame_sizes[FRAME_COUNT];

    if (FRAME_COUNT == 1) frame_sizes[0] = mod;
    else {
        for (int i = 0; i < FRAME_COUNT - 1; i++) frame_sizes[i] = FRAME_SIZE;
        if (mod > FRAME_SIZE / 2) frame_sizes[FRAME_COUNT - 1] = mod;
        else frame_sizes[FRAME_COUNT - 1] = FRAME_SIZE + mod;
    }

    if (THREAD > 1) {
        u32 frame_counter = 0;
        bool state = true;

        for (int i = 0; i < THREAD; i++) BUFFER[i] = new u8[FRAME_SIZE * 2];
        for (int i = 0; i < THREAD; i++) OUT[i] = new u8[FRAME_SIZE * 2];

        while (state) {
            thread threads[THREAD];

            u16 thread_counter = 0;
            for (int i = 0; i < THREAD; i++) {
                OUT_COUNTER[i] = 0;
                fread(BUFFER[i], 1, COMPRESSED_FRAME_SIZES[frame_counter], INPUT_FILE);
                threads[i] = thread(DECODING, BUFFER[i], frame_sizes[frame_counter], i, frame_counter);

                frame_counter++;
                thread_counter++;
                if (frame_counter == FRAME_COUNT) { state = false;  break; }
            }
 
            for (u32 i = 0; i < thread_counter; i++) threads[i].join();
            for (u32 i = 0; i < thread_counter; i++) fwrite(OUT[i], 1, OUT_COUNTER[i], OUTPUT_FILE);
        }

        for (u32 i = 0; i < THREAD; i++) {
            delete[] BUFFER[i];
            delete[] OUT[i];
        }
    }
    else {
        BUFFER[0] = new u8[FRAME_SIZE * 2];
        OUT[0] = new u8[FRAME_SIZE * 2];

        for (int i = 0; i < FRAME_COUNT; i++) {
            OUT_COUNTER[0] = 0;

            fread(BUFFER[0], 1, COMPRESSED_FRAME_SIZES[i], INPUT_FILE);
            DECODING(BUFFER[0], frame_sizes[i], 0, i);
            fwrite(OUT[0], 1, OUT_COUNTER[0], OUTPUT_FILE);
        }

        delete[] BUFFER[0];
        delete[] OUT[0];
    }

    //////////////////////////////////////////  
    if (METADATA_LAST_SIZE > 0) {
        fread(METADATA_LAST, 1, METADATA_LAST_SIZE, INPUT_FILE);
        fwrite(METADATA_LAST, 1, METADATA_LAST_SIZE, OUTPUT_FILE);
    }
    //METADATA_SIZE = fread(METADATA, 1, 1024 * 64, INPUT_FILE);
    //if (METADATA_SIZE > 0) fwrite(METADATA, 1, METADATA_SIZE, OUTPUT_FILE);
    //////////////////////////////////////////

    fclose(INPUT_FILE);
    fclose(OUTPUT_FILE);
}

int main(int argc, char** argv) {
    
    clock_t startTime = clock();
    double time;

    bool RELEASE = true;

    if (RELEASE) {
        ///////////// Reading Console Arguments /////////////
        if (argc < 3) {
            USAGE();
        }
        else {
            INPUT_FILENAME = argv[1];
            OUTPUT_FILENAME = argv[2];

            if (OUTPUT_FILENAME.rfind("wav") == -1) {
                cout << "The input file should be .halac extension and the output file should be .wav extension!";
                exit(0);
            }

            for (int i = 3; i < argc; i++) {
                //cout << argv[i] << endl;
                string param = argv[i];

                if (param.compare(0, 2, "-h") == 0) USAGE();
                else if (param.compare(0, 3, "-ow") == 0) OVERWRITE = 1;
                else if (param.compare(0, 4, "-mt=") == 0) {
                    THREAD = stoi(param.substr(4));
                    if (THREAD < 1) THREAD = 1;
                    else if (THREAD > 1024) THREAD = 1024;
                    //cout << (int)THREAD << endl;
                }
                else {
                    cout << "Parameter error!" << endl;
                    exit(0);
                }
            }

            READ_HALAC_HEADER(INPUT_FILENAME);
            WRITE_WAV_HEADER(OUTPUT_FILENAME);
            FRAMING();
        }

    }
    else {
        //////////////////////////////////////////
        string path;
        //path = "../WAV/ISKENDER_PAYDAS";
        //path = "../WAV/test";
        path = "../WAV/lossy_wav";
        //path = "../WAV/TEST_AUDIO_SQUEEZE_CHART";
        //path = "../WAV/HANDE_YENER";
        //path = "../WAV/SIBEL_CAN";
        //path = "../WAV/TEST_AUDIOS";
        //path = "../WAV/YILDIZ_TILBE";
        //path = "../WAV/single";
        //path = "../WAV/YABANCI/Cybernetika_Solar_Nexus";
        //path = "../WAV/YABANCI/Gubbology_Anachronism";
        //path = "../WAV/YABANCI/Busta_Rhymes";
        //path = "../WAV/YABANCI/Sean_Paul";
        //path = "C:/Qt/Qt_Projects/HALAC_AUDIO_PLAYER/WAV";
        //path = "../WAV/YABANCI/Globular - Entangled Everything - 2018";
        //////////////////////////////////////////

        vector<string> files;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_directory() && (entry.path().extension() == ".halac"))
                files.emplace_back(entry.path().string());
            //cout << entry.path() << endl;
        }
        sort(files.begin(), files.end()); // greater<string>()
        //////////////////////////////////////////

        //TOTAL_OUTPUT_SIZE = 0;
        THREAD = 1;

        for (u32 i = 0; i < files.size(); i++) { // files.size()
            INPUT_FILENAME = files.at(i);
            OUTPUT_FILENAME = INPUT_FILENAME + ".org";
            cout << OUTPUT_FILENAME << endl;
            //cout << OUTPUT_FILENAME.substr(OUTPUT_FILENAME.rfind('\\') + 1) << ":\t";

            //OUTPUT_SIZE = 0;

            READ_HALAC_HEADER(INPUT_FILENAME);
            WRITE_WAV_HEADER(OUTPUT_FILENAME);
            FRAMING();
            //TOTAL_OUTPUT_SIZE += OUTPUT_SIZE;
        }
        //cout << TOTAL_OUTPUT_SIZE << endl;
    }
  
    //////////////////////////////////////////
    time = static_cast<double>(clock() - (double)startTime) / CLOCKS_PER_SEC;
    cout << "Total Time: " << time << " seconds" << endl;
    //////////////////////////////////////////

    return 0;
}

