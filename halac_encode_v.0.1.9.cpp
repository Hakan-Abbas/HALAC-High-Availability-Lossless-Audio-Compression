#include "halac_encode_v.0.1.9.h"

void Forward_Linear_Prediction(double* coeffs, const i32* data, u32 size) {
    int numSamples = size - 1;
    double autocorr[ORDER + 1];

    for (int lag = 0; lag <= ORDER; lag++) {
        autocorr[lag] = 0.0;
        int samples = numSamples - lag;
        do { autocorr[lag] += (double)data[samples] * data[samples + lag]; } while (samples--);
    }

    double lpCoeffs[ORDER + 1];
    lpCoeffs[0] = 1.0;
    for (int i = 1; i <= ORDER; i++) lpCoeffs[i] = 0.0;

    double predError = autocorr[0] != 0.0 ? autocorr[0] : 1.0;

    for (int order = 0; order < ORDER; order++) {
        double reflection = -lpCoeffs[0] * autocorr[order + 1];
        for (int j = 1; j <= order; j++) reflection -= lpCoeffs[j] * autocorr[order + 1 - j];
        reflection /= predError;

        double tempCoeffs[ORDER + 1];
        for (int n = 0; n <= order + 1; n++) tempCoeffs[n] = lpCoeffs[n] + reflection * lpCoeffs[order + 1 - n];
        for (int n = 0; n <= order + 1; n++) lpCoeffs[n] = tempCoeffs[n];

        predError -= predError * reflection * reflection;
    }

    for (int i = 0; i < ORDER; i++) coeffs[i] = lpCoeffs[i + 1];
}

void COMPRESS(i32* data, u32 sample_count, u8* OUT, u32 &OUT_COUNTER) {

    ////////////////////////////
    double coeffs[ORDER];
    Forward_Linear_Prediction(coeffs, data, sample_count);

    float coeffs_float[ORDER];
    for (int i = 0; i < ORDER; i++) coeffs[i] = coeffs_float[i] = coeffs[i];

    double predicted;
    i32 data_error[sample_count];
    for (int i = 0; i < ORDER; i++) data_error[i] = data[i];

    for (int k = ORDER; k < sample_count; k++) {
        predicted = 0.0;
        for (int j = 0; j < ORDER; j++) {
            predicted -= coeffs[j] * data[k - 1 - j];
        }

        data_error[k] = data[k] - (int)predicted;
    }

    memcpy(OUT + OUT_COUNTER, coeffs_float, ORDER * 4);
    OUT_COUNTER += ORDER * 4;
    ////////////////////////////

    u8 data_first[sample_count];
    u8 data_last[sample_count];
    u32 over_data[1024 * 16];
    u16 over_counter = 0;

    for (int i = 0; i < sample_count; i++) {
        u32 value = Signed_To_Unsigned(data_error[i]);

        if (value < 65535) {
            data_first[i] = value >> 8;
            data_last[i] = value;
        }
        else {
            over_data[over_counter] = value - 65535;
            over_counter++;

            data_first[i] = 255;
            data_last[i] = 255;
        }

    }
    memcpy(OUT + OUT_COUNTER, &over_counter, 2);
    OUT_COUNTER += 2;

    if (over_counter > 0) {
        memcpy(OUT + OUT_COUNTER, &over_data, over_counter * 4);
        OUT_COUNTER += over_counter * 4;
    }

    FSE_ENCODE(data_first, sample_count, OUT, OUT_COUNTER);
    HUF_ENCODE(data_last, sample_count, OUT, OUT_COUNTER);
}

void TRANSFORM(i16* data, u32 data_size, u8* OUT, u32 &OUT_COUNTER) {
    ////////////////////////////
    if (OUT_COUNTER > 1024 * 512) {
        fwrite(OUT, 1, OUT_COUNTER, OUTPUT_FILE);
        OUT_COUNTER = 0;
    }

    u32 sample_count = data_size / CHANNEL_COUNT;
    i32 data_left[sample_count];
    i32 data_right[sample_count];
    for (int i = 0; i < sample_count; i++) {
        data_left[i] = data[2 * i];
        data_right[i] = data[2 * i + 1];
    }
    ////////////////////////////
    int L, R, X, Y;
    for (int i = 0; i < sample_count; i++) {
        L = data_left[i];
        R = data_right[i];
        X = L - R;
        Y = R + (X / 2);
        data_left[i] = X;
        data_right[i] = Y;
    }
    ////////////////////////////
    i32 data_left_delta[sample_count];
    i32 data_right_delta[sample_count];
    data_left_delta[0] = data_left[0];
    data_right_delta[0] = data_right[0];

    for (int i = 1; i < sample_count; i++) {
        data_left_delta[i] = data_left[i] - data_left[i - 1];
        data_right_delta[i] = data_right[i] - data_right[i - 1];
    }
    ////////////////////////////

    COMPRESS(data_left_delta, sample_count, OUT, OUT_COUNTER);
    COMPRESS(data_right_delta, sample_count, OUT, OUT_COUNTER);
}

void ENCODING() {
    READ_WAV_HEADER();
    WRITE_HALAC_HEADER();
    //////////////
    u8 OUT[1024 * 1024];
    u32 OUT_COUNTER = 0;

    u32 data_mod = INPUT_SIZE % BLOCK_SIZE;
    u32 block_count = INPUT_SIZE / BLOCK_SIZE - 1;
    u32 last_block_size;

    if (block_count <= 0) last_block_size = INPUT_SIZE;
    else last_block_size = BLOCK_SIZE + data_mod;

    i16 data[BLOCK_SIZE];
    for (int i = 0; i < block_count; i++) {
        fread(data, 2, BLOCK_SIZE / SAMPLE_BYTE, INPUT_FILE);
        TRANSFORM(data, BLOCK_SIZE / SAMPLE_BYTE, OUT, OUT_COUNTER);
    }

    fread(data, 2, last_block_size / SAMPLE_BYTE, INPUT_FILE);
    TRANSFORM(data, last_block_size / SAMPLE_BYTE, OUT, OUT_COUNTER);

    if (OUT_COUNTER > 0) {
        fwrite(OUT, 1, OUT_COUNTER, OUTPUT_FILE);
    }

    //////////////
    fclose(INPUT_FILE);
    fclose(OUTPUT_FILE);
}

int main(int argc, char** argv)
{
    /*
    ///////////// Reading Console Arguments /////////////
    if (argc != 3) {
        printf(VERSION);
        printf(" Encoder\n"
            "Hakan Abbas (abbas.hakan@gmail.com)\n"
            "Only 16 bit, 2 channels .wav types are supported for input!\n"
            "Please use this format: \"encoder.exe input_file output_file\"\n");
        return 0;
    }
    INPUT_FILENAME = argv[1];
    OUTPUT_FILENAME = argv[2];
    ENCODING();
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
        if (!entry.is_directory() && (entry.path().extension() == ".wav"))
            files.emplace_back(entry.path().string());
    }
    sort(files.begin(), files.end());
    //////////////////////////////////////////

    TOTAL_OUTPUT_SIZE = 0;
    for (u32 i = 0; i < files.size(); i++) {
        INPUT_FILENAME = files.at(i);
        OUTPUT_FILENAME = INPUT_FILENAME.substr(0, INPUT_FILENAME.rfind('.')) + ".halac";
        cout << OUTPUT_FILENAME << endl;

        OUTPUT_SIZE = 0;
        ENCODING();
        TOTAL_OUTPUT_SIZE += OUTPUT_SIZE;
    }
    cout << TOTAL_OUTPUT_SIZE << endl;

    //////////////////////////////////////////
    time = static_cast<double>(clock() - (double)startTime) / CLOCKS_PER_SEC;
    printf("-----------\n");
    printf("Total Time: %2.3f seconds\n", time);
    //////////////////////////////////////////
    
    return 0;
}
