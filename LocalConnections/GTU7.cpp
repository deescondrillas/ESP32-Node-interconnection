#include "GTU7.h"

void setup() {
  Serial.begin(115200);         // Initialize serial monitor
  gps_init();                   // Initialize GPS
  oled_init();                  // Initialize the OLED display
  wifi_connect();               // Connect to WiFi
  mqtt_init();                  // Set server, buffer and callback
}

void loop() {
    wifi_connect();             // Reconnect to WiFi if it gets disconnected.
    mqtt_connect();             // Connect to the MQTT client
    mqttClient.loop();          // Call the loop to maintain connection to the server.
    gps_read();                 // Store data in the corresponding variables
    serial_gps();               // Show GPS data in the serial monitor
    display_gps();              // Show GPS data in OLED display
    mqtt_publish(channelID);    // MQTT Publish
}
