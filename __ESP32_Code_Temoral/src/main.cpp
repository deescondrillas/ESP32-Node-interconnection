#include "wifi_manager.h"
#include "mqtt_client.h"
#include "config.h"

unsigned long lastSend = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    connectWiFi();
    setupMQTT();
}

void loop() {
    loopMQTT();

    unsigned long now = millis();
    if (now - lastSend > SEND_INTERVAL) {
        publishData();
        lastSend = now;
    }
}
