#include "config.h"

// WiFi
const char* WIFI_SSID     = "CharlyA34";
const char* WIFI_PASSWORD = "carlos13";

// MQTT / ThingSpeak
const char* MQTT_SERVER     = "mqtt3.thingspeak.com";
const int   MQTT_PORT       = 1883;  // non-secure for now
const char* MQTT_USER       = "JQs9ByoEBi0RBSMtCxgRBwI";
const char* MQTT_CLIENT_ID  = "JQs9ByoEBi0RBSMtCxgRBwI";
const char* MQTT_PASS       = "LPvMo6GgwXSs3JO4apF49akX";


// Device ID
const String DEVICE_ID = "ESP32_01";  // later we can auto-generate from MAC

// Timing
const unsigned long SEND_INTERVAL = 20000; // ms
