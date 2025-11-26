#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <JPEGDecoder.h>
#include <Arduino.h>

#include "config.h"

// ST7735 pins
#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/************************************************
 *                GLOBAL VARIABLES
 ***********************************************/
long lastFetch = 0;
int fetchInterval = 5000; // Fetch plot every 5 seconds

/************************************************
 *                FUNCTION PROTOTYPES
 ***********************************************/
void app_setup();
void app_loop();

void connectWiFi();
void fetchAndDisplayPlot();

/************************************************
 *                SETUP FUNCTION
 ***********************************************/
void app_setup() {
    Serial.begin(115200);

    // Initialize ST7735
    tft.initR(INITR_144GREENTAB);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);

    // Connect to WiFi
    connectWiFi();
}

/************************************************
 *                LOOP FUNCTION
 ***********************************************/
void app_loop() {
    // Fetch plot periodically
    if (millis() - lastFetch > fetchInterval) {
        lastFetch = millis();
        fetchAndDisplayPlot();
    }
}

/************************************************
 *                WIFI FUNCTIONS
 ***********************************************/
void connectWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID_VAR, WIFI_PASSWORD_VAR);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

/************************************************
 *        FETCH AND DISPLAY PLOT
 ***********************************************/
void fetchAndDisplayPlot() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("Fetching plot from server...");

    HTTPClient http;
    http.begin(PLOT_URL);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Failed to fetch plot, HTTP code: %d\n", httpCode);
        http.end();
        return;
    }

    // Get the payload
    WiFiClient* stream = http.getStreamPtr();
    size_t len = stream->available();
    if (len == 0) {
        Serial.println("No data received from server");
        http.end();
        return;
    }

    uint8_t* buffer = (uint8_t*)malloc(len);
    if (!buffer) {
        Serial.println("Failed to allocate buffer");
        http.end();
        return;
    }

    stream->readBytes(buffer, len);

    // Decode JPEG (or converted PNG->JPEG)
    // JpegDec.decodeArray(buffer, len);
    free(buffer);

    if (JpegDec.width == 0 || JpegDec.height == 0) {
        Serial.println("JPEG decoding failed!");
        http.end();
        return;
    }

    tft.fillScreen(ST77XX_BLACK);
    // JpegDec.render(0, 0);
    Serial.println("Plot displayed!");

    http.end();
}
