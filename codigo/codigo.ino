/*
- ESP32-Boards version: 3.0.7 
- **Nota: No funciono la captura del audio con la board esp32 version 2.0.5 que es la que suelo usar, si grababa pero audio en blanco.) usar version 3.0.7
- Board: ESP32 Dev Module
*/
#include <SPIFFS.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_task_wdt.h"

const char* ssid = "WILLIAN-2.4G-pJAt";
const char* password = "942900777";
const char* UPLOAD_ENDPOINT = "http://192.168.18.42:8000/upload";

#define I2S_WS 25
#define I2S_SD 34
#define I2S_SCK 26
#define SAMPLE_RATE 16000 // 16 kilohertz (kHz) 
#define TARGET_SIZE 524288 //512kb - 16seconds
#define BUFFER_SIZE 1024

#define REC_LED 27
#define BLT_LED 14

int32_t i2sBuffer[BUFFER_SIZE];


void setup() {
    pinMode(REC_LED, OUTPUT);
    pinMode(BLT_LED, OUTPUT);
  
    Serial.begin(115200);
    delay(1000);
    esp_task_wdt_deinit();

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println(" Connected!");

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed, formatting...");
        if (!SPIFFS.format() || !SPIFFS.begin(true)) { Serial.println("SPIFFS not available"); return; }
    }

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false
    };
    i2s_pin_config_t pin_config = { .bck_io_num = I2S_SCK, .ws_io_num = I2S_WS, .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = I2S_SD };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);

    SPIFFS.remove("/audio.wav");
    File file = SPIFFS.open("/audio.wav", FILE_WRITE);
    uint8_t wavHeader[44] = {0};
    file.write(wavHeader, 44);

    size_t totalBytes = 0;
    Serial.println("Recording audio...");
    
    digitalWrite(REC_LED, HIGH);
    
    while (totalBytes < TARGET_SIZE) {
        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, (char*)i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);
        int samplesRead = bytesRead / sizeof(int32_t);
        for (int i = 0; i < samplesRead; i++) {
            int16_t sample16 = (int16_t)(i2sBuffer[i] >> 14);
            file.write((uint8_t*)&sample16, sizeof(sample16));
            totalBytes += sizeof(sample16);
            if (totalBytes >= TARGET_SIZE) break;
        }
    }

    file.seek(0);
    uint32_t dataSize = totalBytes, fileSize = dataSize + 36, byteRate = SAMPLE_RATE * 2;
    uint8_t header[44] = {
        'R','I','F','F',
        (uint8_t)(fileSize & 0xFF), (uint8_t)((fileSize >> 8) & 0xFF),
        (uint8_t)((fileSize >> 16) & 0xFF), (uint8_t)((fileSize >> 24) & 0xFF),
        'W','A','V','E','f','m','t',' ', 16,0,0,0, 1,0, 1,0,
        (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF),
        (uint8_t)((SAMPLE_RATE >> 16) & 0xFF), (uint8_t)((SAMPLE_RATE >> 24) & 0xFF),
        (uint8_t)(byteRate & 0xFF), (uint8_t)((byteRate >> 8) & 0xFF),
        (uint8_t)((byteRate >> 16) & 0xFF), (uint8_t)((byteRate >> 24) & 0xFF),
        2,0, 16,0,
        'd','a','t','a',
        (uint8_t)(dataSize & 0xFF), (uint8_t)((dataSize >> 8) & 0xFF),
        (uint8_t)((dataSize >> 16) & 0xFF), (uint8_t)((dataSize >> 24) & 0xFF)
    };
    file.write(header, 44);
    file.close();
    Serial.println("Recording complete");

    digitalWrite(REC_LED, LOW);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(UPLOAD_ENDPOINT);
        
        http.addHeader("Content-Type", "audio/wav");
        File wavFile = SPIFFS.open("/audio.wav", FILE_READ);
        int code = http.sendRequest("POST", &wavFile, wavFile.size());
        wavFile.close();
        if (code > 0) Serial.printf("File uploaded, server response: %d\n", code);
        else Serial.printf("Upload error: %s\n", http.errorToString(code).c_str());
        http.end();
    }
   
}

void loop() {}
