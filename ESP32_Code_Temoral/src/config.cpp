#include "config.h"

// WiFi
const char* WIFI_SSID     = "Tec-IoT";
const char* WIFI_PASSWORD = "spotless.magnetic.bridge";

// MQTT (Mosquitto Local)

const char* MQTT_SERVER   = "192.168.1.74";   // <-- Cambialo CHARLS!!!!!!!!!!!!!
const int   MQTT_PORT     = 1883;

const char* MQTT_USER     = "";     // sin auth
const char* MQTT_PASS     = "";     // sin auth

const char* MQTT_CLIENT_ID = "ESP32_TEST";


// Device

const String DEVICE_ID = "ESP32_01";

// Necesario porque mqttPublish() recibe este parámetro
const long channelID = 0;


// Timings
const unsigned long SEND_INTERVAL = 20000; // 20 segundos

