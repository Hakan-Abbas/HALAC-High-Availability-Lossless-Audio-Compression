#include "halac_encode_v.0.2.9.h"
//#include "RICE/rice.h"

void ForwardLinearPrediction(double* coeffs, const i32* data, u32 size, const u8 ORDER) {
    int N = size - 1;

    double R[ORDER + 1];
    for (int i = 0; i <= ORDER; i++) {
        R[i] = 0.0;
        for (int j = 0; j <= N - i; j++) {
            R[i] += data[j] * data[j + i];
        }
    }

    double Ak[ORDER + 1];
    double Ek = R[0];
    if (Ek == 0) Ek = 1;

    for (int i = 0; i <= ORDER; i++) {
        Ak[i] = 0.0;
    }
    Ak[0] = 1.0;

    for (int k = 0; k < ORDER; k++) {
        double lambda = 0.0;
        for (int j = 0; j <= k; j++) {
            lambda -= Ak[j] * R[k + 1 - j];
        }
        lambda /= Ek;

        for (int n = 0; n <= (k + 1) / 2; n++) {
            double temp = Ak[k + 1 - n] + lambda * Ak[n];
            Ak[n] = Ak[n] + lambda * Ak[k + 1 - n];
            Ak[k + 1 - n] = temp;
        }

        Ek *= 1.0 - lambda * lambda;
    }

    for (int i = 0; i < ORDER; i++) {
        coeffs[i] = Ak[i + 1];
    }
}

void COMPRESS(i32* data, u32 sample_count, u32 FRAME_INDEX) {
    u64 abs_error1 = 0, abs_error2 = 0;

    for (int i = 2; i < sample_count; i+=8) abs_error1 += ABS(data[i] - data[i - 1]);

    ////////////////////////////
    for (int i = 2; i < sample_count; i+=8) {
        i32 P = 2 * data[i - 1] - data[i - 2];
        abs_error2 += ABS(data[i] - P);
    }
    ////////////////////////////
    i32 data_delta[sample_count];
    u8 side_info = 0;

    if (abs_error1 < abs_error2) {
        //side_info |= 1;
        data_delta[0] = data[0];
        for (int i = 1; i < sample_count; i++) {            
            data_delta[i] = data[i] - data[i - 1];
        }
    }
    else {
        side_info |= 1;

        data_delta[0] = data[0];
        data_delta[1] = data[1];
        for (int i = 2; i < sample_count; i++) {
            i32 P = 2 * data[i - 1] - data[i - 2];
            data_delta[i] = data[i] - P;
        }
    }
    ////////////////////////////

    double coeffs[32];
    u8 coeffs_u8[32];
    //float16 coeffs_float16[32];
    i32 data_error[sample_count];
    double predicted;
    ////////////////////////////
    const double min_value = -2.56; // Min float
    const double max_value = 2.54; // Max float
    ////////////////////////////

    if (MODE == 0) { // "-normal"
        const u8 order = 8;
        ForwardLinearPrediction(coeffs, data_delta, sample_count, order);
        for (int i = 0; i < order; i++) {
            //coeffs[i] = coeffs_float16[i] = coeffs[i];
            int integer = coeffs[i] * 100;
            if (abs(integer) % 2 == 1) {
                //cout << integer << " ";
                if (integer < 0) integer--;
                else integer++;
            }

            //cout << integer << " ";
            coeffs[i] = (integer / 100.0);
            //cout << coeffs[i] << endl;

            if (coeffs[i] < min_value) coeffs[i] = min_value;
            if (coeffs[i] > max_value) coeffs[i] = max_value;

            coeffs_u8[i] = (coeffs[i] - min_value) / (max_value - min_value) * 255.0;
        }

        for (int i = 0; i < order; i++) {
            data_error[i] = data_delta[i]; //cout << coeffs[i] << "->" << (int)coeffs_u8[i] << " ";
        }
        //cout << endl;

        for (int k = order; k < sample_count; k++) {
            predicted = 0.0;
            for (int j = 0; j < order; j++) {
                predicted -= (coeffs[j] * 1.00001) * data_delta[k - 1 - j];
                //if (x % 2 == 0) cout << k << " " << coeffs[j] << " " << data_delta[k - 1 - j] << " " << predicted << endl;
            }

            //if (x % 2 == 0) cout << k << " " << predicted << endl;            

            predicted = clampMax(predicted, 65535);
            predicted = clampMin(predicted, -65536);
            
            data_error[k] = data_delta[k] - predicted;            
        }

        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], coeffs_u8, order); OUT_COUNTER[FRAME_INDEX] += order;
    }
    else if (MODE == 1) { // "-fast"
        const u8 order = 2;
        ForwardLinearPrediction(coeffs, data_delta, sample_count, order);
        for (int i = 0; i < order; i++) {
            int integer = coeffs[i] * 100;
            if (abs(integer) % 2 == 1) {
                //cout << integer << " ";
                if (integer < 0) integer--;
                else integer++;
            }

            //cout << integer << " ";
            coeffs[i] = (integer / 100.0);

            if (coeffs[i] < min_value) coeffs[i] = min_value;
            if (coeffs[i] > max_value) coeffs[i] = max_value;

            coeffs_u8[i] = (coeffs[i] - min_value) / (max_value - min_value) * 255.0;
        }

        for (int i = 0; i < order; i++) data_error[i] = data_delta[i];

        for (int k = order; k < sample_count; k++) {
            predicted = 0.0;
            for (int j = 0; j < order; j++) {
                predicted -= (coeffs[j] * 1.00001) * data_delta[k - 1 - j];
            }

            predicted = clampMax(predicted, 65535);
            predicted = clampMin(predicted, -65536);

            data_error[k] = data_delta[k] - predicted;
        }
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], coeffs_u8, order); OUT_COUNTER[FRAME_INDEX] += order;
    }
    else if (MODE == 2) {
        memcpy(data_error, data_delta, sample_count * 4);
    }

    ////////////////////////////
    //RICE_ENCODE(data_error, sample_count, FRAME_INDEX);
    ////////////////////////////

    // FSE & HUF////////////////
    /**/
    u32 data_error_unsigned[sample_count];
    u32 MAX = data_error_unsigned[0] = S2U(data_error[0]);
    u32 value;

    for (int i = 1; i < sample_count; i++) {
        value = data_error_unsigned[i] = S2U(data_error[i]); //cout << value << " " << BIT_COUNT(value) << endl;
        if (value > MAX) MAX = value;
    }
    ////////////////////////////

    u8 max_bit_count = BIT_COUNT(MAX);
    side_info |= (max_bit_count << 1);

    if (max_bit_count <= 8) { //cout << "FSE " << (int)max_bit_count << endl;
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &side_info, 1); OUT_COUNTER[FRAME_INDEX] += 1;

        u8 data_last[sample_count];
        for (int i = 0; i < sample_count; i++) data_last[i] = data_error_unsigned[i] & 255;

        if (MODE != 1) FSE_ENCODE(data_last, sample_count, FRAME_INDEX);
        else HUF_ENCODE(data_last, sample_count, FRAME_INDEX);
    }
    else if (max_bit_count <= 16) { //cout << "FSE + HUF " << (int)max_bit_count << endl;
        u8 data_first[sample_count];
        u8 data_last[sample_count];

        for (int i = 0; i < sample_count; i++) {
            data_first[i] = data_error_unsigned[i] >> 8;
            data_last[i] = data_error_unsigned[i] & 255;
        }

        double entropy = ENTROPY(data_last, sample_count / 4);
        if (entropy < 2) side_info |= (1 << 6);

        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &side_info, 1); OUT_COUNTER[FRAME_INDEX] += 1;

        if (MODE != 1) {
            FSE_ENCODE(data_first, sample_count, FRAME_INDEX);
            if (entropy < 2) FSE_ENCODE(data_last, sample_count, FRAME_INDEX);
            else HUF_ENCODE(data_last, sample_count, FRAME_INDEX);
        }
        else {
            HUF_ENCODE(data_first, sample_count, FRAME_INDEX);
            HUF_ENCODE(data_last, sample_count, FRAME_INDEX);
        }
        //cout << (int)max_bit_count << " " << entropy << " " << (int)side_info << endl;
    }
    else { //cout << "FSE + HUF + OVER" << endl;
        u8 data_first[sample_count];
        u8 data_last[sample_count];
        u32 over_data[1024 * 16];
        u16 over_counter = 0;

        for (int i = 0; i < sample_count; i++) {
            u32 value = data_error_unsigned[i];

            if (value < 65535) {
                data_first[i] = value >> 8;
                data_last[i] = value & 255;
            }
            else {
                over_data[over_counter] = value - 65535;
                over_counter++;

                data_first[i] = 255;
                data_last[i] = 255;
            }
        }
        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &side_info, 1); OUT_COUNTER[FRAME_INDEX] += 1;

        memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &over_counter, 2); OUT_COUNTER[FRAME_INDEX] += 2;
        if (over_counter > 0) {
            memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &over_data, over_counter * 4);
            OUT_COUNTER[FRAME_INDEX] += over_counter * 4;
        }            

        if (MODE != 1) FSE_ENCODE(data_first, sample_count, FRAME_INDEX);
        else HUF_ENCODE(data_first, sample_count, FRAME_INDEX);
        HUF_ENCODE(data_last, sample_count, FRAME_INDEX);
    }
    
    ////////////////////////////
}

void TRANSFORM(i16* data, u32 data_size, u32 FRAME_INDEX) {
    u32 sample_count = data_size / CHANNEL_COUNT;
    i32 data_left[sample_count];
    i32 data_right[sample_count];

    for (int i = 0; i < sample_count; i++) {
        data_left[i] = data[2 * i];
        data_right[i] = data[2 * i + 1];
    }
    ////////////////////////////
    
    if (MODE != 2) {
        int L, R, X, Y;
        for (int i = 0; i < sample_count; i++) {
            L = data_left[i];
            R = data_right[i];
            X = L - R;
            Y = R + (X / 2);
            data_left[i] = X;
            data_right[i] = Y;
        }
    }

    COMPRESS(data_left, sample_count, FRAME_INDEX);
    COMPRESS(data_right, sample_count, FRAME_INDEX);

}

void ENCODING(u8* BUFFER, u32 FRAME_SIZE, u32 FRAME_INDEX) {
    //////////////
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &VERSION[6], 1); OUT_COUNTER[FRAME_INDEX] += 1;
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &VERSION[8], 1); OUT_COUNTER[FRAME_INDEX] += 1;
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &VERSION[10], 1); OUT_COUNTER[FRAME_INDEX] += 1;
    //////////////
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &MODE, 1); OUT_COUNTER[FRAME_INDEX] += 1;
    //////////////
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &CHANNEL_COUNT, 1); OUT_COUNTER[FRAME_INDEX] += 1;
    //////////////
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &SAMPLE_BIT, 1); OUT_COUNTER[FRAME_INDEX] += 1;
    //////////////
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &SAMPLE_RATE, 3); OUT_COUNTER[FRAME_INDEX] += 3;
    //////////////
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &FRAME_SIZE, 3); OUT_COUNTER[FRAME_INDEX] += 3;
    //////////////
    OUT_COUNTER[FRAME_INDEX] += 3; // for compressed frame size
    //////////////
    u32 HASH = (u32)XXH3_64bits(BUFFER, FRAME_SIZE); //cout << HASH << endl;
    memcpy(OUT[FRAME_INDEX] + OUT_COUNTER[FRAME_INDEX], &HASH, 4); OUT_COUNTER[FRAME_INDEX] += 4;
    //////////////

    u32 BLOCK_SIZE;
    if (MODE == 0) BLOCK_SIZE = 1024 * 16; // MODE 0 -> "-normal"
    else if (MODE == 1) BLOCK_SIZE = 1024 * 32; // MODE 1 -> "-fast"
    else if (MODE == 2) BLOCK_SIZE = 1024 * 8; // MODE 2 -> "-lossywav"

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

    i16 data[BLOCK_SIZE * 2];
    u32 POS = 0;

    for (int i = 0; i < block_count; i++) { //block_count
        u32 data_size = block_sizes[i] / SAMPLE_BYTE;
        //cout << data_size << endl;
        memcpy(data, BUFFER + POS, data_size * 2);
        POS += data_size * 2;
        TRANSFORM(data, data_size, FRAME_INDEX);
    }
}

void FRAMING() {
    u32 division = DATA_SIZE / FRAME_SIZE;
    u32 mod = DATA_SIZE % FRAME_SIZE;
    u32 frame_count;
    u32 frame_sizes[division + 1];

    if (division == 0) {
        frame_count = 1;
        frame_sizes[0] = mod;
    }
    else {
        frame_count = division;
        for (int i = 0; i < frame_count; i++) frame_sizes[i] = FRAME_SIZE;
        if (mod > FRAME_SIZE / 2) {
            frame_sizes[frame_count] = mod;
            frame_count++;
        }
        else {
            frame_sizes[frame_count - 1] = FRAME_SIZE + mod;
        }
    }

    //////////////////////////////////////////
    fwrite(&frame_count, 1, 4, OUTPUT_FILE);
    fseek(OUTPUT_FILE, frame_count * 4, SEEK_CUR);
    //////////////////////////////////////////

    u32 frame_counter = 0;
    bool state = true;
    u32 counter = 0;

    if (THREAD > 1) {
        for (int i = 0; i < THREAD; i++) BUFFER[i] = new u8[FRAME_SIZE * 2];
        for (int i = 0; i < THREAD; i++) OUT[i] = new u8[FRAME_SIZE * 2];

        while (state) {
            thread threads[THREAD];

            u16 thread_counter = 0;
            for (int i = 0; i < THREAD; i++) {
                OUT_COUNTER[i] = 0;
                fread(BUFFER[i], 1, frame_sizes[frame_counter], INPUT_FILE);
                //cout << frame_counter << " " << thread_counter << endl;     

                threads[i] = thread(ENCODING, BUFFER[i], frame_sizes[frame_counter], i);

                frame_counter++;
                thread_counter++;
                if (frame_counter == frame_count) { state = false;  break; }
            }

            for (u32 i = 0; i < thread_counter; i++) threads[i].join();
            for (u32 i = 0; i < thread_counter; i++) {
                memcpy(OUT[i] + 12, &OUT_COUNTER[i], 3); // // for compressed frame size
                fwrite(OUT[i], 1, OUT_COUNTER[i], OUTPUT_FILE);                

                COMPRESSED_FRAME_SIZES[counter] = OUT_COUNTER[i]; //cout << counter << " " << OUT_COUNTER[i] << endl;
                counter++;
            }
        }

        for (u32 i = 0; i < THREAD; i++) {
            delete[] BUFFER[i];
            delete[] OUT[i];
        }
    }
    else {
        BUFFER[0] = new u8[FRAME_SIZE * 2];
        OUT[0] = new u8[FRAME_SIZE * 2];

        for (int i = 0; i < frame_count; i++) { // frame_count
            OUT_COUNTER[0] = 0;
            fread(BUFFER[0], 1, frame_sizes[i], INPUT_FILE);
            ENCODING(BUFFER[0], frame_sizes[i], 0);

            memcpy(OUT[0] + 12, &OUT_COUNTER[0], 3); // // for compressed frame size
            fwrite(OUT[0], 1, OUT_COUNTER[0], OUTPUT_FILE); //cout << OUT_COUNTER[0] << endl;            

            COMPRESSED_FRAME_SIZES[i] = OUT_COUNTER[0]; //cout << OUT_COUNTER[0] << endl;
        }

        delete[] BUFFER[0];
        delete[] OUT[0];
    }

    //////////////////////////////////////////
    METADATA_LAST_SIZE = fread(METADATA_LAST, 1, 1024 * 64, INPUT_FILE);
    if (METADATA_LAST_SIZE > 0) {
        fwrite(METADATA_LAST, 1, METADATA_LAST_SIZE, OUTPUT_FILE);
        fseek(OUTPUT_FILE, 75 + LOSSYWAV_SIZE, SEEK_SET);
        fwrite(&METADATA_LAST_SIZE, 1, 2, OUTPUT_FILE);
    }

    fseek(OUTPUT_FILE, 81 + LOSSYWAV_SIZE + METADATA_FIRST_SIZE, SEEK_SET);
    fwrite(COMPRESSED_FRAME_SIZES, 4, frame_count, OUTPUT_FILE);

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
            INPUT_FILENAME = argv[1]; //cout << INPUT_FILENAME << endl;
            OUTPUT_FILENAME = argv[2];        

            if (OUTPUT_FILENAME.rfind("halac") == -1) {
                cout << "The input file should be .wav extension and the output file should be .halac extension!";
                exit(0);
            }

            for (int i = 3; i < argc; i++) {
                string param = argv[i];

                if (param.compare(0, 2, "-h") == 0) USAGE();
                else if (param.compare(0, 3, "-ow") == 0) OVERWRITE = 1;
                else if (param.compare(0, 5, "-fast") == 0) MODE = 1;
                else if (param.compare(0, 4, "-mt=") == 0) {
                    THREAD = stoi(param.substr(4));
                    if (THREAD < 1) THREAD = 1;
                    else if (THREAD > 1024) THREAD = 1024;
                }
                else {
                    cout << "Parameter error!" << endl;
                    exit(0);
                }
            }

            READ_WAV_HEADER(INPUT_FILENAME);
            WRITE_HALAC_HEADER(OUTPUT_FILENAME);
            FRAMING();
        }
        /////////////////////////////////////////////////////
    }
    else {
        //////////////////////////////////////////
        string path;
        //path = "../WAV/ISKENDER_PAYDAS";
        //path = "../WAV/test";
        //path = "../WAV/lossy_wav";
        //path = "../WAV/TEST_AUDIO_SQUEEZE_CHART";
        path = "../WAV/HANDE_YENER";
        //path = "../WAV/SIBEL_CAN";
        //path = "../WAV/TEST_AUDIOS";
        //path = "../WAV/YILDIZ_TILBE";
        //path = "../WAV/single";
        //path = "../WAV/YABANCI/Cybernetika_Solar_Nexus";
        //path = "../WAV/YABANCI/Gubbology_Anachronism";
        //path = "C:/Qt/Qt_Projects/HALAC_AUDIO_PLAYER/WAV";
        //path = "../WAV/YABANCI/Busta_Rhymes";
        //path = "../WAV/YABANCI/Sean_Paul";
        //path = "../WAV/YABANCI/Globular - Entangled Everything - 2018";
        //////////////////////////////////////////

        vector<string> files;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_directory() && (entry.path().extension() == ".wav"))
                files.emplace_back(entry.path().string());
            //cout << entry.path() << endl;
        }
        sort(files.begin(), files.end()); // greater<string>()
        //////////////////////////////////////////
        TOTAL_OUTPUT_SIZE = 0;        
        THREAD = 1;

        for (u32 i = 0; i < files.size(); i++) { // files.size()
            INPUT_FILENAME = files.at(i);
            OUTPUT_FILENAME = INPUT_FILENAME.substr(0, INPUT_FILENAME.rfind('.')) + ".halac";
            cout << OUTPUT_FILENAME << endl;
            //cout << OUTPUT_FILENAME.substr(OUTPUT_FILENAME.rfind('\\') + 1) << ":\t";

            OUTPUT_SIZE = 0;

            MODE = 0; // 0:"-normal", 1:"-fast", 2:"-lossywav"
            READ_WAV_HEADER(INPUT_FILENAME);
            WRITE_HALAC_HEADER(OUTPUT_FILENAME);
            FRAMING();

            //cout << OUTPUT_SIZE << endl;
            TOTAL_OUTPUT_SIZE += OUTPUT_SIZE;
        }
        cout << TOTAL_OUTPUT_SIZE << endl;
    }

    //////////////////////////////////////////
    time = static_cast<double>(clock() - (double)startTime) / CLOCKS_PER_SEC;
    cout << "Total Time: " << time << " seconds" << endl;
    //////////////////////////////////////////

    return 0;
}
