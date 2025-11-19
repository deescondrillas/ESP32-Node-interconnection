#include "config.h"

// WiFi
const char* WIFI_SSID     = "Tec-IoT";
const char* WIFI_PASSWORD = "spotless.magnetic.bridge";

// MQTT / ThingSpeak
const char* MQTT_SERVER     = "192.168.x.x";   // IP de tu laptop
const char* MQTT_USER       = "";
const char* MQTT_CLIENT_ID  = "";
const char* MQTT_PASS       = "";


// Device ID
const String DEVICE_ID = "ESP32_01";  // later we can auto-generate from MAC

// Timing
const unsigned long SEND_INTERVAL = 20000; // ms
