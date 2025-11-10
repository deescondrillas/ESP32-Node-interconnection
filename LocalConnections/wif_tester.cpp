#include <WiFi.h>


// WiFi set-up variables and credentials
const char* ssid = "Tec-IoT";                   // Network name
const char* pass = "spotless.magnetic.bridge";  // Network password

// Function to connect the specified WiFi network
void wifi_connect();

void setup() {
  Serial.begin(115200);
}

void loop() {
  wifi_connect();
}

void wifi_connect() {
  if(WiFi.status() == WL_CONNECTED) return;
  Serial.println("Connecting...\n");
  while(WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(3000);
    Serial.println(WiFi.status());
  }
  Serial.println("Connected to WiFi.");
}
