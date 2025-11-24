#pragma once
#include <Arduino.h>
#include "passwords.h"

// WiFi
const char* WIFI_SSID_VAR     = "Tec-IoT";                      //Network name
const char* WIFI_PASSWORD_VAR = "spotless.magnetic.bridge";     //Network password

// MQTT / ThingSpeak
const char* MQTT_SERVER   = "";         // MQTT Server direction - local IPv4
const int   MQTT_PORT     = 1883;       // Non-secure MQTT
const char* MQTT_USER_VAR      = "";    // MQTT passwords - Non required for now
const char* MQTT_CLIENT_ID_VAR = "";
const char* MQTT_PASS_VAR      = "";

// Channel ID
#define CHANNEL_ID "test"               // Define channel as test channel

// Device ID
const char* DEVICE_ID = "ESP32_01";
