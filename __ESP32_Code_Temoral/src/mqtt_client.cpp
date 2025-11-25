#include "mqtt_client.h"
#include "config.h"
#include "sensors.h"
#include <WiFi.h>
#include <PubSubClient.h>

// Cliente TCP normal (no seguro)
WiFiClient espClient;
PubSubClient mqttClient(espClient);


//CONECTAR AL BROKER LOCAL

void connectMQTT() {

    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");

        // Conexión simple sin usuario/contraseña
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.print("MQTT connected to ");
            Serial.print(MQTT_SERVER);
            Serial.print(" on port ");
            Serial.println(MQTT_PORT);

        } else {
            Serial.print(" failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 2 seconds...");
            delay(2000);
        }
    }
}


//CONFIGURAR MQTT

void setupMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    connectMQTT();
}


//LOOP MQTT

void loopMQTT() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}


//PUBLICAR DATOS
 
bool mqttPublish(long pubChannelID,
                 float rssi, float down, float up, 
                 float lat, float lon, String deviceID) 
{
    // Construir un solo mensaje
    String message =
        "rssi=" + String(rssi) +
        "&down=" + String(down) +
        "&up=" + String(up) +
        "&lat=" + String(lat, 5) +
        "&lon=" + String(lon, 5) +
        "&device=" + deviceID;

    // Topic único en Mosquitto local
    String topicString = "esp32/data";

    bool ok = mqttClient.publish(topicString.c_str(), message.c_str());

    Serial.println("Topic: " + topicString);
    Serial.println("Payload: " + message);
    Serial.println(ok ? "✅ Publish OK" : "❌ Publish FAILED");

    return ok;
}

//OBTENER Y ENVIAR DATOS

void publishData() {
    updateGPS();

    float rssi = getRSSI();
    float down = getThroughputDown();
    float up   = getThroughputUp();
    float lat  = getGPSLat();
    float lon  = getGPSLon();

    mqttPublish(channelID, rssi, down, up, lat, lon, DEVICE_ID);
}

