#pragma once
#include <Arduino.h>

// WiFi
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// MQTT
extern const char* MQTT_SERVER;
extern const int   MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASS;
extern const char* MQTT_CLIENT_ID;

// Device
extern const String DEVICE_ID;
extern const long channelID;

// Timing
extern const unsigned long SEND_INTERVAL;

