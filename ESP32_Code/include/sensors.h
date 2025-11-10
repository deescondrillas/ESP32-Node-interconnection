#pragma once
#include <HTTPClient.h>
#include <WiFi.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

const char* DOWNLOAD_URL;
const char* UPLOAD_URL;

void initGPS();
void updateGPS();
float getRSSI();
float getThroughputDown();
float getThroughputUp();
float getGPSLat();
float getGPSLon();
