#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <TinyGPSPlus.h>
#include <TimeLib.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <Wire.h>

#include "config.h"   // WiFi + MQTT credentials

/************************************************
 *                GLOBAL VARIABLES
 ***********************************************/

// -------- GPS --------
TinyGPSPlus gps;                    // GT-U7 GPS definition
HardwareSerial SerialGPS(1);        // Create new serial port for GPS

float latitude{0}, longitude{0};    // Variables for storing the coordinates
int sat{0}, rx{16}, tx{17};         // Physical serial ports used and satellites


// Spacial reference variables
const double EARTH_RADIUS = 6371000.0;                  // Earth radius in meters
const double ONE_DEG = EARTH_RADIUS * 2 * M_PI / 360;   // One rotation degree in meters
const double REF_LAT = 19.01620;                        // Reference latitude in degrees
const double REF_LON = -98.24581;                       // Reference longitude in degrees

// Time variables
int yy{0}, mm{0}, dd{0}, hs{0}, mins{0}, secs{0};

// Timer helpers
// Sensor printing and publishing intervals
int delayPub{10}, delayPrint{3};    // Sensor readings are published every 10 seconds and printed every 3
long lastPub{0}, lastPrint{0};      // To hold the value of last call of the millis() function

// -------- MQTT Client --------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

/************************************************
 *                FUNCTION PROTOTYPES
 ***********************************************/
void connectWiFi();

float getRSSI();
float getThroughputDown();
float getThroughputUp();

void gps_init();
void gps_read();
void serial_gps();
void get_meters();
void connectMQTT();
void setupMQTT();
void loopMQTT();
String buildMQTTMessage();
void publishData();

void app_setup();
void app_loop();

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

float getRSSI() {
    return WiFi.RSSI();
}

float getThroughputDown() {
    // Checks wifi connection
    if (WiFi.status() != WL_CONNECTED) return 0.0;

    // Creates an HTTP request
    HTTPClient http;
    http.begin(DOWNLOAD_URL);

    // Times the download
    uint32_t start = millis();
    int httpCode = http.GET();

    // Checks for a succesful HTTP response
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Download failed, HTTP code: %d\n", httpCode);
        http.end();
        return 0.0;
    }

    // Gets the bytestream
    WiFiClient* stream = http.getStreamPtr();
    // It will read the data un 512byte chunks
    uint8_t buffer[512];
    size_t totalBytes = 0;
    int bytesRead = 0;
    // Continues until the server closes the connection
    while (http.connected() && (bytesRead = stream->readBytes(buffer, sizeof(buffer))) > 0) {
        totalBytes += bytesRead;
    }

    // Calculate total time
    uint32_t duration = millis() - start;
    http.end();
    // Convert to MBPs
    float bits = totalBytes * 8.0;
    float seconds = duration / 1000.0;
    float mbps = (bits / seconds) / 1e6;

    Serial.printf("Download: %.2f Mbps (%d bytes in %lu ms)\n", mbps, totalBytes, duration);
    return mbps;
}

float getThroughputUp() {
    // Checks wifi connection
    if (WiFi.status() != WL_CONNECTED) return 0.0;

    //Starts an HTTP post request
    HTTPClient http;
    http.begin(UPLOAD_URL);  // LAN IP of your PC

    // Creates a dummy buffer (100 KB)
    const size_t uploadSize = 100 * 1024;
    uint8_t* buffer = (uint8_t*)malloc(uploadSize);
    if (!buffer) return 0.0;
    memset(buffer, 0, uploadSize);  //Fills the buffer with 0s

    // Times the upload
    uint32_t start = millis();
    int httpCode = http.POST(buffer, uploadSize);
    uint32_t duration = millis() - start;

    //Clean up
    free(buffer);
    http.end();

    // Checks if upload succeded
    if (httpCode != 200) {
        Serial.printf("Upload failed, code: %d\n", httpCode);
        return 0.0;
    }

    //Converts to Mbps
    float bits = uploadSize * 8.0;
    float seconds = duration / 1000.0;
    float mbps = (bits / seconds) / 1e3;

    Serial.printf("Upload: %.2f Mbps (%d bytes in %lu ms)\n", mbps, uploadSize, duration);
    return mbps;
}

/************************************************
 *                GPS FUNCTIONS
 ***********************************************/
 void gps_init() {
     delay(1500);
     Serial.println("\nInitializing GPS...");
     SerialGPS.begin(9600, SERIAL_8N1, tx, rx);
     while(SerialGPS.available()) SerialGPS.read();
     delay(500);
 }

void gps_read() {
    do {
        //Read data from GPS
        while(SerialGPS.available()) {
            char c = SerialGPS.read();
            gps.encode(c);
        }
        //Store data
        sat = gps.satellites.value();
        latitude  = gps.location.lat();
        longitude = gps.location.lng();
        yy = gps.date.year();
        mm = gps.date.month();
        dd = gps.date.day();
        hs = gps.time.hour();
        mins = gps.time.minute();
        secs = gps.time.second();

        //Check connection
        if (!gps.location.isValid()) {
            Serial.println("GPS no signal...");
            delay(3000);
        }
    //Repeat while not connected
    } while (!gps.location.isValid());

    //Convert to meters
    get_meters();
}

void get_meters() {
    // Conversion constants
    const double m_per_deg_lat = ONE_DEG;                                  // meters per degree latitude
    const double m_per_deg_lon = ONE_DEG * cos(REF_LAT * (M_PI / 180.0));  // meters per degree longitude at refLat

    // Differences in degrees
    double dLat = latitude  - REF_LAT;
    double dLon = longitude - REF_LON;

    // Conversion to meters
    double north_m = dLat * m_per_deg_lat;
    double east_m  = dLon * m_per_deg_lon;

    // Overwrite global variables
    latitude  = (float)north_m;
    longitude = (float)east_m;
}

void serial_gps() {
    if(millis() - lastPrint > 1000 * delayPrint) {
        lastPrint = millis();
        Serial.print("\nLat: "); Serial.println(latitude, 5);
        Serial.print("Lon: "); Serial.println(longitude, 5);
        Serial.print("Satellites: "); Serial.println(sat);
    }
}

/************************************************
 *                MQTT FUNCTIONS
 ***********************************************/
void connectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID_VAR, MQTT_USER_VAR, MQTT_PASS_VAR)) {
            Serial.println("OK");
        } else {
            Serial.print("Failed, rc=");
            Serial.println(mqttClient.state());
            delay(2000);
        }
    }
}

void setupMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    connectMQTT();
}

void loopMQTT() {
    if (!mqttClient.connected()) connectMQTT();
    mqttClient.loop();
}

String buildMQTTMessage() {
    float rssi = getRSSI();
    float down = getThroughputDown();
    float up   = getThroughputUp();

    setTime(hs, mins, secs, dd, mm, yy);
    time_t date = now() - 6 * 3600;

    String msg =
        "field1=" + String(rssi) +
        "&field2=" + String(down) +
        "&field3=" + String(up) +
        "&field4=" + String(latitude) +
        "&field5=" + String(longitude) +
        "&field6=" + String(date) +
        "&field7=" + String(DEVICE_ID);

    return msg;
}

void publishData() {
    if(millis() - lastPub > 1000 * delayPub) {
        lastPub = millis();
        String payload = buildMQTTMessage();
        String topic = String("channels/") + CHANNEL_ID + "/publish";

        Serial.println(payload);
        mqttClient.publish(topic.c_str(), payload.c_str());
    }
}

/************************************************
 *                MAIN APP LOGIC
 ***********************************************/
void app_setup() {
    Serial.begin(115200);
    gps_init();
    connectWiFi();
    setupMQTT();
    Serial.println("System Ready.");
}

void app_loop() {
    loopMQTT();
    gps_read();
    serial_gps();
    publishData();
}
