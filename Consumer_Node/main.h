#pragma once

// ---------- LIBRARIES ----------
#include <Adafruit_SSD1306.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>

#include "config.h"        // WiFi + MQTT credentials

/************************************************
 *                GLOBAL VARIABLES
 ***********************************************/

// -------- MQTT Client --------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// -------- OLED 128x32 (I2C 21/22) --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET   -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Last MQTT payload received
String lastPayload = "No data yet";
bool   newPayload  = false;

// Parsed values for nicer display
String lastDeviceId = "";
String lastRssiStr  = "";
String lastDownStr  = "";
String lastUpStr    = "";
String lastTimeStr  = "";

unsigned long lastUnixSeconds = 0;
unsigned long lastUnixMillis  = 0;
unsigned long lastOledRefresh = 0;

// -------- LCD 128x160 (TFT) --------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define LCD_WIDTH   160
#define LCD_HEIGHT  128
#define IMG_BYTES   (LCD_WIDTH * LCD_HEIGHT * 2)
uint8_t imgBuf[IMG_BYTES];

unsigned long lastPlotFetch = 0;
const unsigned long PLOT_FETCH_INTERVAL_MS = 5000;   // refresh plot every 5s

/************************************************
 *           FUNCTION PROTOTYPES
 ***********************************************/

// WiFi
void connectWiFi();
float getRSSI();

// MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectMQTT();
void setupMQTT();
void loopMQTT();

// OLED / LCD helpers
void setupOLED();
void drawOLED();
void setupLCD();
bool fetchAndDrawPlot();

// Payload parsing
String getValue(String data, char separator, int index);
void parsePayload(const String& payload);
String formatClock();

// “Publisher-style” functions (kept as no-op just
// so main.cpp / structure matches, but not used)
float getThroughputDown();
float getThroughputUp();
String buildMQTTMessage();
void publishData();

// App entry points
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

// Consumer does not need throughput tests
float getThroughputDown() { return 0.0f; }
float getThroughputUp()   { return 0.0f; }


/************************************************
 *                OLED FUNCTIONS
 ***********************************************/
void setupOLED() {
    // I2C pins: SDA=21, SCL=22
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        for (;;);  // halt
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Consumer ready...");
    display.display();
}

void drawOLED() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Line 1: Device & Time
    display.setCursor(0, 0);
    if (lastDeviceId.length() > 0) {
        display.print(lastDeviceId);
    } else {
        display.print("Device?");
    }

    String clockStr = formatClock();
    display.print(" ");
    display.print(clockStr);

    // Line 2: Down / Up   (or truncated raw payload if parse fails)
    display.setCursor(0, 16);
    if (lastDownStr.length() > 0 || lastUpStr.length() > 0) {
        display.print("Dn:");
        display.print(lastDownStr);
        display.print(" Up:");
        display.print(lastUpStr);
    } else {
        String shortPayload = lastPayload.substring(0, 21);
        display.print(shortPayload);
    }

    display.display();
}


/************************************************
 *                LCD FUNCTIONS
 ***********************************************/
 void setupLCD() {
     tft.initR(INITR_BLACKTAB);
     tft.setRotation(3);
     tft.fillScreen(ST77XX_BLACK);
     tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
     tft.setTextSize(1);
     tft.setCursor(0, 0);
     tft.println("Waiting for plot...");
 }

// Fetches http://<server>:8080/plot.bin and draws it on LCD
bool fetchAndDrawPlot() {
    Serial.println("[LCD] fetchAndDrawPlot() called");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[LCD] No WiFi, cannot fetch plot.");
        return false;
    }

    HTTPClient http;
    http.begin(PLOT_BIN_URL);
    Serial.print("[LCD] GET "); Serial.println(PLOT_BIN_URL);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[LCD] HTTP plot error: %d -> %s\n",
                      httpCode,
                      http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t totalRead = 0;

    while (http.connected() && totalRead < IMG_BYTES) {
        size_t available = stream->available();
        if (available) {
            int toRead = min((size_t)available, IMG_BYTES - totalRead);
            int readNow = stream->readBytes(imgBuf + totalRead, toRead);
            if (readNow <= 0) break;
            totalRead += readNow;
        } else {
            delay(1);
        }
    }

    http.end();

    Serial.printf("[LCD] Bytes read: %d (expected %d)\n",
                  (int)totalRead, IMG_BYTES);

    if (totalRead != IMG_BYTES) {
        Serial.println("[LCD] Incomplete image, aborting draw.");
        return false;
    }

    uint16_t* pixels = (uint16_t*)imgBuf;

    tft.drawRGBBitmap(0, 0, pixels, LCD_WIDTH, LCD_HEIGHT);

    Serial.println("[LCD] Plot drawn on LCD.");
    return true;
}

/************************************************
 *          MQTT PAYLOAD PARSING
 ***********************************************/
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    if (found > index) {
        return data.substring(strIndex[0], strIndex[1]);
    }
    return "";
}

void parsePayload(const String& payload) {
    // field1=RSSI, field2=down, field3=up, field7=DEVICE_ID
    String f1 = getValue(payload, '&', 0);
    String f2 = getValue(payload, '&', 1);
    String f3 = getValue(payload, '&', 2);
    String f6 = getValue(payload, '&', 5);
    String f7 = getValue(payload, '&', 6);

    int idx;

    idx = f1.indexOf('=');
    if (idx >= 0) lastRssiStr = f1.substring(idx + 1);

    idx = f2.indexOf('=');
    if (idx >= 0) lastDownStr = f2.substring(idx + 1);

    idx = f3.indexOf('=');
    if (idx >= 0) lastUpStr = f3.substring(idx + 1);

    idx = f6.indexOf('=');
    if (idx >= 0) {
        lastTimeStr = f6.substring(idx + 1);
        unsigned long tVal = lastTimeStr.toInt();
        if (tVal > 0) {
            lastUnixSeconds = tVal;
            lastUnixMillis  = millis();
        }
    }

    idx = f7.indexOf('=');
    if (idx >= 0) lastDeviceId = f7.substring(idx + 1);
}

String formatClock() {
    if (lastUnixSeconds == 0) {
        return String("--:--:--");
    }

    // seconds elapsed since last MQTT msg
    unsigned long current = lastUnixSeconds;

    int sec  = current % 60;
    int min  = (current / 60) % 60;
    int hour = (current / 3600) % 24;

    char buf[9];
    sprintf(buf, "%02d:%02d:%02d", hour, min, sec);
    return String(buf);
}

/************************************************
 *                MQTT FUNCTIONS
 ***********************************************/
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    Serial.print("MQTT message [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(msg);

    lastPayload = msg;
    parsePayload(lastPayload);
    newPayload = true;   // tell loop to refresh OLED
}

void connectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        // Use MQTT_CLIENT_ID_VAR from config.h (should be unique per node)
        if (mqttClient.connect(MQTT_CLIENT_ID_VAR, MQTT_USER_VAR, MQTT_PASS_VAR)) {
            Serial.println("OK");

            // Subscribe to the same topic publishers send to:
            String topic = String("channels/") + CHANNEL_ID + "/publish";
            mqttClient.subscribe(topic.c_str());
            Serial.print("Subscribed to: ");
            Serial.println(topic);
        } else {
            Serial.print("Failed, rc=");
            Serial.println(mqttClient.state());
            delay(2000);
        }
    }
}

void setupMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
}

void loopMQTT() {
    if (!mqttClient.connected()) connectMQTT();
    mqttClient.loop();
}


/************************************************
 *     PUBLISHER-STYLE FUNCTIONS (NO-OP HERE)
 ***********************************************/
String buildMQTTMessage() {
    // Consumer does not publish
    return "";
}

void publishData() {
    // Consumer does not publish
}


/************************************************
 *                MAIN APP LOGIC
 ***********************************************/
void app_setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n[Consumer] Booting...");

    setupOLED();
    setupLCD();
    connectWiFi();
    setupMQTT();

    Serial.println("[Consumer] System Ready.");

    bool ok = fetchAndDrawPlot();
    Serial.printf("[Consumer] Initial plot fetch: %s\n", ok ? "OK" : "FAILED");
}

void app_loop() {
    loopMQTT();

    unsigned long now = millis();

    // Refresh OLED every 1 second with ticking clock
    if (now - lastOledRefresh > 1000) {
        lastOledRefresh = now;
        drawOLED();
    }

    // Periodically fetch the plot from the Flask server
    if (now - lastPlotFetch > PLOT_FETCH_INTERVAL_MS) {
        lastPlotFetch = now;
        fetchAndDrawPlot();
    }

    delay(10);
}
