#include "GT-U7.h"

void setup() {
  Serial.begin(115200);   // Initialize serial monitor
  gps_init();             // Initialize GPS
  oled_init();            // Initialize the OLED display
  wifi_connect();         // Connect to WiFi

  // Configure the MQTT client to connect with ThingSpeak broker and properly handle messages
  mqttClient.setServer(server, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(2048);
}


void loop() {
    // Reconnect to WiFi if it gets disconnected.
    wifi_connect();

    // Connect to the MQTT client
    if (!mqttClient.connected()) {
        mqttConnect();
        mqttSubscribe(channelID);
    }

    // Call the loop to maintain connection to the server.
    mqttClient.loop();

    // Store data in the corresponding variables
    gps_read();

    // Show GPS data in the serial monitor
    serial_gps();

    // Show GPS data in OLED display
    display_gps();

    // MQTT Publish
    mqttPublish(channelID);
}
