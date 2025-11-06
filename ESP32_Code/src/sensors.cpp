#include "sensors.h"
#include "config.h"

// Set your test server address (LAN preferred)
// Change to local IPv4!
const char* DOWNLOAD_URL = "http://10.79.148.74:8080/testfile.bin";
const char* UPLOAD_URL = "http://10.79.148.74:8080/upload";

float getRSSI() {
    return WiFi.RSSI();
}

float getThroughputDown() {
    if (WiFi.status() != WL_CONNECTED) return 0.0;

    HTTPClient http;
    http.begin(DOWNLOAD_URL);
    uint32_t start = millis();
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Download failed, HTTP code: %d\n", httpCode);
        http.end();
        return 0.0;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[512];
    size_t totalBytes = 0;
    int bytesRead = 0;

    while (http.connected() && (bytesRead = stream->readBytes(buffer, sizeof(buffer))) > 0) {
        totalBytes += bytesRead;
    }

    uint32_t duration = millis() - start;
    http.end();

    float bits = totalBytes * 8.0;
    float seconds = duration / 1000.0;
    float mbps = (bits / seconds) / 1e6;

    Serial.printf("Download: %.2f Mbps (%d bytes in %lu ms)\n", mbps, totalBytes, duration);
    return mbps;
}

float getThroughputUp() {
    if (WiFi.status() != WL_CONNECTED) return 0.0;

    HTTPClient http;
    http.begin(UPLOAD_URL);  // LAN IP of your PC

    // Create a dummy buffer (e.g., 100 KB of zeros)
    const size_t uploadSize = 100 * 1024;
    uint8_t* buffer = (uint8_t*)malloc(uploadSize);
    if (!buffer) return 0.0;
    memset(buffer, 0, uploadSize);

    uint32_t start = millis();
    int httpCode = http.POST(buffer, uploadSize);
    uint32_t duration = millis() - start;

    free(buffer);
    http.end();

    if (httpCode != 200) {
        Serial.printf("Upload failed, code: %d\n", httpCode);
        return 0.0;
    }

    float bits = uploadSize * 8.0;
    float seconds = duration / 1000.0;
    float mbps = (bits / seconds) / 1e6;

    Serial.printf("Upload: %.2f Mbps (%d bytes in %lu ms)\n", mbps, uploadSize, duration);
    return mbps;
}

float getGPSLat() {
    // Fake GPS (Tec de Monterrey Puebla)
    return 19.0184;
}

float getGPSLon() {
    return -98.2421;
}
