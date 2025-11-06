#pragma once
#include <PubSubClient.h>
#include <String>

void connectMQTT();
void setupMQTT();
void loopMQTT();
bool mqttPublish(long pubChannelID,
                 float rssi, float down, float up, float lat, float lon,
                 String deviceID);
void publishData();
