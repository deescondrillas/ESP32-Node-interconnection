#include "mqtt_client.h"
#include "config.h"
#include "sensors.h"

#ifdef USESECUREMQTT
  #include <WiFiClientSecure.h>
  WiFiClientSecure espClient;
#else
  #include <WiFi.h>
  WiFiClient espClient;
#endif

PubSubClient mqttClient(espClient);

//Certificate
#ifdef USESECUREMQTT
const char * PROGMEM thingspeak_ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIGzDCCBbSgAwIBAgIQCN5DWKzI0uORoAibwxNwUDANBgkqhkiG9w0BAQsFADBP\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMSkwJwYDVQQDEyBE\n" \
"aWdpQ2VydCBUTFMgUlNBIFNIQTI1NiAyMDIwIENBMTAeFw0yNTA1MjkwMDAwMDBa\n" \
"Fw0yNjA0MTQyMzU5NTlaMG4xCzAJBgNVBAYTAlVTMRYwFAYDVQQIEw1NYXNzYWNo\n" \
"dXNldHRzMQ8wDQYDVQQHEwZOYXRpY2sxHDAaBgNVBAoTE1RoZSBNYXRod29ya3Ms\n" \
"IEluYy4xGDAWBgNVBAMMDyoubWF0aHdvcmtzLmNvbTCCASIwDQYJKoZIhvcNAQEB\n" \
"BQADggEPADCCAQoCggEBAMPgMMbT3eb+bjyZE9tvMuyhv7hLGkbTXrc4TA23m3kv\n" \
"Gt+lqwoz/PDmUZin/JeqWZFhmBBok0miHMZ5DUsPBapLjyIScuZcudIrShwhLzQO\n" \
"r9BnvDbpec7/6QOgL8sZnYOrE3nCLsjbl2DMmpB6X6nfuTH+C1KvIz+Ile9c/kdj\n" \
"M5MJM+MQKU+61EhN3ULg5fSjfiyOQEABIw2kvan2BEmZWZ4Aj4PTO6tGIB55ji4o\n" \
"RApOW9KNPz2jmKWvxvJhIXXI/VXq0Xa9CSEUW3JDs++kVhBjv5MG2bBihzqpLyXi\n" \
"SkJFpS4+oP6YXLc01NdKq6W1nAQ0+PGzQNo/zrLweEUCAwEAAaOCA4MwggN/MB8G\n" \
"A1UdIwQYMBaAFLdrouqoqoSMeeq02g+YssWVdrn0MB0GA1UdDgQWBBSnJYIJUG+1\n" \
"dGn8SpPCY3moPRBUGzApBgNVHREEIjAggg8qLm1hdGh3b3Jrcy5jb22CDW1hdGh3\n" \
"b3Jrcy5jb20wPgYDVR0gBDcwNTAzBgZngQwBAgIwKTAnBggrBgEFBQcCARYbaHR0\n" \
"cDovL3d3dy5kaWdpY2VydC5jb20vQ1BTMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUE\n" \
"FjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwgY8GA1UdHwSBhzCBhDBAoD6gPIY6aHR0\n" \
"cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0VExTUlNBU0hBMjU2MjAyMENB\n" \
"MS00LmNybDBAoD6gPIY6aHR0cDovL2NybDQuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0\n" \
"VExTUlNBU0hBMjU2MjAyMENBMS00LmNybDB/BggrBgEFBQcBAQRzMHEwJAYIKwYB\n" \
"BQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBJBggrBgEFBQcwAoY9aHR0\n" \
"cDovL2NhY2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0VExTUlNBU0hBMjU2MjAy\n" \
"MENBMS0xLmNydDAMBgNVHRMBAf8EAjAAMIIBgAYKKwYBBAHWeQIEAgSCAXAEggFs\n" \
"AWoAdwCWl2S/VViXrfdDh2g3CEJ36fA61fak8zZuRqQ/D8qpxgAAAZcc3H67AAAE\n" \
"AwBIMEYCIQCMXO8IIgr18WJQzsJwhVA28nQ0pw4Z9ageZ8B2mkzHvgIhAK2/SJR/\n" \
"JlwcE+59EdzWuQrjUF2a8LGFem170mauPRi2AHcAZBHEbKQS7KeJHKICLgC8q08o\n" \
"B9QeNSer6v7VA8l9zfAAAAGXHNx+rAAABAMASDBGAiEAups1qd8UcSBD4SvRc3Hl\n" \
"a6CEtKFNTbYSqMiO44IRIZoCIQCjv2v7mcuk4OQLGIzqSW7s5GYZUg2hZoRxGpM7\n" \
"1fcupAB2AEmcm2neHXzs/DbezYdkprhbrwqHgBnRVVL76esp3fjDAAABlxzcfsMA\n" \
"AAQDAEcwRQIgXHy8PiRzIwTuSgAfjzYfrcZIzSpkxK4XSgh30bif+3gCIQCcm/zQ\n" \
"GNtt5wFfaaUc20Fav24Z0ZXGVkDzDpcCvQn1DzANBgkqhkiG9w0BAQsFAAOCAQEA\n" \
"aY0ZcolcQ+RLXIABRPGho1ppF3pzHYdExhprNSTh1fQXJKG7mAdm04EB4vjZVyCJ\n" \
"ckZphm25MtOtfwjb9b+BK9s2ZPOyGk5m0EspVZ9fk8wSqxMFaaeLgTuW+oxcu0mh\n" \
"C+c2lq3Q900QrZ9DNwW0tiSaof8bU1yPvEkxFAErxu/Ro2mnCU6IJcFPiXaVgL8T\n" \
"sle5fecgrOdC1bwCRrUKBLiNyPPeIwbDNYbHw0CI8rb6u8X45h9RCw8PoEEDS5eX\n" \
"H6PeHGsAk7V7TZ2JG75NFOeZbmEJ8qn74PVTQECfTTSnEB3NWgxDvNaMido2PA3z\n" \
"4nwXVf8ZyUmg57O/Br67LA==\n" \
"-----END CERTIFICATE-----\n";
#endif


void connectMQTT() {

    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
              Serial.print( "MQTT to " );
              Serial.print( MQTT_SERVER );
              Serial.print (" at port ");
              Serial.print( MQTT_PORT );
              Serial.println( " successful." );
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 2s...");
            delay(2000);
        }
    }
}

void setupMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    #ifdef USESECUREMQTT
        espClient.setCACert(thingspeak_ca_cert);
    #endif
    connectMQTT();
}

void loopMQTT() {
    if (!mqttClient.connected()) connectMQTT();
    mqttClient.loop();
}

bool mqttPublish(long pubChannelID,
                 float rssi, float down, float up, float lat, float lon,
                 String deviceID) {
  String message = "field1=" + String(rssi) +
                   "&field2=" + String(down) +
                   "&field3=" + String(up) +
                   "&field4=" + String(lat, 5) +
                   "&field5=" + String(lon, 5) +
                   "&field6=" + String(deviceID);

  String topicString = "channels/" + String(pubChannelID) + "/publish";
  bool ok = mqttClient.publish(topicString.c_str(), message.c_str());
  Serial.println("Topic: " + topicString);
  Serial.println("Payload: " + message);
  Serial.println(ok ? "✅ Publish OK" : "❌ Publish failed");
  return ok;
}

void publishData() {
    float rssi = getRSSI();
    float down = getThroughputDown();
    float up = getThroughputUp();
    float lat = getGPSLat();
    float lon = getGPSLon();

    mqttPublish(channelID, rssi, down, up, lat, lon, DEVICE_ID);

}
