#pragma once
#include <Arduino.h>
#include "passwords.h"

// WiFi
const char* WIFI_SSID_VAR     = "Totalplay-149F";
const char* WIFI_PASSWORD_VAR = "149F5250gtYFVWDx";

// MQTT / ThingSpeak
const char* MQTT_SERVER   = "mqtt3.thingspeak.com";
const int   MQTT_PORT     = 1883;  // Non-secure MQTT
const char* MQTT_USER_VAR      = "JQs9ByoEBi0RBSMtCxgRBwI";
const char* MQTT_CLIENT_ID_VAR = "JQs9ByoEBi0RBSMtCxgRBwI";
const char* MQTT_PASS_VAR      = "LPvMo6GgwXSs3JO4apF49akX";

// Channel ID
#define CHANNEL_ID 3150959

// Device ID
const char* DEVICE_ID = "ESP32_01";
