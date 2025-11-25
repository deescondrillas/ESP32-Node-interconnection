#pragma once
#include <Arduino.h>

// WiFi
//const char* WIFI_SSID_VAR     = "";
//const char* WIFI_PASSWORD_VAR = "149F5250gtYFVWDx";
const char* WIFI_SSID_VAR     = "Tec-IoT";                      //Network name
const char* WIFI_PASSWORD_VAR = "spotless.magnetic.bridge";     //Network password

// WiFi test servers
const char* DOWNLOAD_URL = "http://10.50.77.144:8080/testfile.bin"; //Local IPv4 Adress w/ port
const char* UPLOAD_URL   = "http://10.50.77.144:8080/upload";       //Local IPv4 adrees w/ port

// MQTT credentials
const char* MQTT_SERVER   = "10.50.77.144";     // MQTT Server direction - local IPv4
const int   MQTT_PORT     = 1883;               // Non-secure MQTT
const char* MQTT_USER_VAR      = "";            // MQTT passwords - Non required for now
const char* MQTT_CLIENT_ID_VAR = "";
const char* MQTT_PASS_VAR      = "";

// Channel ID
const char* CHANNEL_ID = "test";        // Define channel as test channel

// Device ID
const char* DEVICE_ID = "ESP32_01";     // Unique machine identifier
