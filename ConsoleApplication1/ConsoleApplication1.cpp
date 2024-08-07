#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <bitset>
#include <algorithm>

using namespace std;
using namespace cv;

const size_t HEADER_SIZE = 32;
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 512;
const size_t FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT;
const int NUM_FRAMES = 1000;

// коррекция пикселей
void correctionOfPixels(const vector<vector<vector<uint16_t>>>& input, vector<vector<vector<uint8_t>>>& output){
    uint16_t minVal;
    uint16_t maxVal;
    float gamma;
    vector<vector<vector<uint8_t>>> justFrame (NUM_FRAMES,vector<vector<uint8_t>>(FRAME_HEIGHT, vector<uint8_t>(FRAME_WIDTH * 3)));

    // 16-ти разрядные значения в 8-ми разрядные
    for (int frameIdx = 0; frameIdx < NUM_FRAMES; frameIdx++) {
        minVal = 65535;
        maxVal = 0;
        for (int y = 0; y < FRAME_HEIGHT; ++y) {
            for (int x = 0; x < FRAME_WIDTH; ++x) {
                minVal = min(minVal, input[frameIdx][y][x]);
                maxVal = max(maxVal, input[frameIdx][y][x]);
            }
        }
      
        for (int y = 0; y < FRAME_HEIGHT; ++y) {
            for (int x = 0; x < FRAME_WIDTH; ++x) {
                uint8_t value = (maxVal > minVal) ?
                    static_cast<uint8_t>(((input[frameIdx][y][x] - minVal) / static_cast<float>(maxVal - minVal)) * 255) : 0;
                justFrame[frameIdx][y][x * 3] = value;     
                justFrame[frameIdx][y][x * 3 + 1] = value; 
                justFrame[frameIdx][y][x * 3 + 2] = value; 
            }
        }
        
       // cout << "MIN " << minVal << " Max " << maxVal;

     // гамма-коррекция, коэффициент гамма методом научного тыка определяю таким образом:
        gamma = 0.0001017 * (maxVal - minVal) + 0.755;
        //cout << " gamma " << gamma << endl;
        for (int y = 0; y < FRAME_HEIGHT; ++y) {
            for (int x = 0; x < FRAME_WIDTH; ++x) {
                uint8_t r = justFrame[frameIdx][y][x * 3];
                uint8_t g = justFrame[frameIdx][y][x * 3 + 1];
                uint8_t b = justFrame[frameIdx][y][x * 3 + 2];
    // использую формулу O=I^(1/G), где I принадлежит [0..1]
                float normR = r / 255.0f;
                float normG = g / 255.0f;
                float normB = b / 255.0f;

                float correctedR = pow(normR, 1.0f / gamma);
                float correctedG = pow(normG, 1.0f / gamma);
                float correctedB = pow(normB, 1.0f / gamma);

                output[frameIdx][y][x * 3] = static_cast<uint8_t>(correctedR * 255);
                output[frameIdx][y][x * 3 + 1] = static_cast<uint8_t>(correctedG * 255);
                output[frameIdx][y][x * 3 + 2] = static_cast<uint8_t>(correctedB * 255);
            }
        }
    }
}

int main() {
    setlocale(LC_ALL, "ru");

    vector<vector<vector<uint16_t>>> frames_16(NUM_FRAMES, vector<vector<uint16_t>>(FRAME_HEIGHT, vector<uint16_t>(FRAME_WIDTH)));
    vector<vector<vector<uint8_t>>> frames_8_corrected(NUM_FRAMES, vector<vector<uint8_t>>(FRAME_HEIGHT, vector<uint8_t>(FRAME_WIDTH * 3)));
    const string filename = "C:/dump_13122019_145433.bin";

    // создание объекта для записи видео файла
    VideoWriter video("test1.mp4", VideoWriter::fourcc('M', 'J', 'P', 'V'), 45, Size(FRAME_WIDTH, FRAME_HEIGHT), true);
    if (!video.isOpened()) {
        cerr << "Ошибка при открытии файла для записи видео." << endl;
        return 1;
    }

    // открытие файла 
    ifstream infile(filename, ios::binary);
    if (!infile) {
        cerr << "Ошибка при открытии файла: " << filename << endl;
        return 1;
    }

    // пропустить первые 32 байта
    infile.seekg(HEADER_SIZE, ios::beg);
 
    // чтение файла
    for (int frameIdx = 0; frameIdx < NUM_FRAMES; ++frameIdx) {
        for (int y = 0; y < FRAME_HEIGHT; ++y) {
            for (int x = 0; x < FRAME_WIDTH; ++x) {
                uint16_t pixelValue_16;
                infile.read(reinterpret_cast<char*>(&pixelValue_16), sizeof(pixelValue_16));
                frames_16[frameIdx][y][x] = pixelValue_16;
            }
        }
    }
    if (infile) {
        cout << "Успешно прочитаны все кадры." << endl;
    }
    else {
        cerr << "Ошибка при чтении файла." << endl;
    }

    // коррекция пикселей
    correctionOfPixels(frames_16, frames_8_corrected);
 
    // запись в видеофайл скорректированных кадров
    for (int frameIdx = 0; frameIdx < NUM_FRAMES; ++frameIdx) {
        vector<uint8_t> flatData(FRAME_WIDTH * FRAME_HEIGHT * 3);
        for (int y = 0; y < FRAME_HEIGHT; ++y) {
            for (int x = 0; x < FRAME_WIDTH; ++x) {
                flatData[(y * FRAME_WIDTH + x) * 3] = frames_8_corrected[frameIdx][y][x * 3];
                flatData[(y * FRAME_WIDTH + x) * 3 + 1] = frames_8_corrected[frameIdx][y][x * 3 + 1];
                flatData[(y * FRAME_WIDTH + x) * 3 + 2] = frames_8_corrected[frameIdx][y][x * 3 + 2];
            }
        }
        Mat image(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3, flatData.data());
        if (image.empty() || image.type() != CV_8UC3) {
            cerr << "Ошибка при создании матрицы изображения для записи в видео." << endl;
            return 1;
        }
        video.write(image);
    }
    video.release();

    cout << "Видео успешно записано." << endl;

    return 0;
}