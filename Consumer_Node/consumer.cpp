#include "GTU7.h"

void setup() {
  Serial.begin(115200);         // Initialize serial monitor
  wifi_connect();               // Connect to WiFi

}

void loop() {
    wifi_connect();             // Reconnect to WiFi if it gets disconnected.
}
