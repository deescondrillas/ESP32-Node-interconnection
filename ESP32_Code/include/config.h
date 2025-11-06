#pragma once
#include <Arduino.h>

// WiFi Configuration
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// ThingSpeak MQTT Configuration
extern const char* MQTT_SERVER;
extern const int   MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASS;
extern const char* MQTT_CLIENT_ID;
extern const char* THINGSPEAK_TOPIC;

// Device Identification
extern const String DEVICE_ID;

// Timings
extern const unsigned long SEND_INTERVAL;

// Channel ID
#define channelID 3150959
