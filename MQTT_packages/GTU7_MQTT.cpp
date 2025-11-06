/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢ 1) ESP32 reads location and timestamp fron GT-U7 through the digital pins RX0 & TX0   ▢
 ▢ 2) ESP32 sends data to the built-in serial port to the host computer (115200)         ▢
 ▢ 3) ESP32 sends the temperature reading to an attached OLED display (SSD1306) if       ▢
 ▢    avalaible, through the I²C bus: (SDA --> GP21, SCL --> GP22)                       ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                                 Macros and Libraries                                  ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */


#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>
#include <PubSubClient.h>
#include <TinyGPS.h>
#include <Wire.h>

// Selection between secure and nonsecure connection as it is hardware-dependent
//#define USESECUREMQTT             // Comment this line if nonsecure connection is used
#ifdef USESECUREMQTT
  #include <WiFiClientSecure.h>
  #define mqttPort 8883
  WiFiClientSecure client;
#else
  #include <WiFi.h>
  #define mqttPort 1883
  WiFiClient client;
#endif

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                               Variables and Definitions                               ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

// Set up the GPS sensor and its variables
#define COORD_FAILED TinyGPS::GPS_INVALID_F_ANGLE
#define SAT_FAILED TinyGPS::GPS_INVALID_SATELLITES
TinyGPS gps;                      // GT-U7 GPS definition
HardwareSerial SerialGPS(1);      // Create new serial port for GPS
float latitude{0}, longitude{0};  // Variables for storing the coordinates
int sat{0}, rx{16}, tx{17};       // Physical serial ports used and satellites

// Variables for storing time
int year{0};
byte month{0}, day{0}, hour{0}, minute{0}, second{0}, hundredths{0};

// Set up the SSD1306 display connected to I2C (SDA, SCL)
#define SCREEN_WIDTH 128          // OLED display width  (pixels)
#define SCREEN_HEIGHT 32          // OLED display height (pixels)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor reading and publishing variables
int updateInterval = 10;          // Sensor readings are published every 10 seconds
long lastUpdate = 0;              // To hold the value of last call of the millis() function

// WiFi set-up variables and credentials
int connectionDelay = 3;          // Delay (s) between trials to connect to WiFi
int status = WL_IDLE_STATUS;      // Initial state of the wifi connection
const char* ssid = " 𝗙𝗿𝗮𝗻𝗰𝗼";     // Network name
const char* pass = "12346789";    // Network password

// ThinkSpeak credentials – account and the defined channels
const char* mqttUserName   = "FSIPNTozNBIMIzMSDAUfDj0";
const char* clientID       = "FSIPNTozNBIMIzMSDAUfDj0";
const char* mqttPass       = "AxH7xNM6mYufHIcV8B59V7gN";

// MQTT broker server
const char* server = "mqtt3.thingspeak.com";

// The MQTT client is liked to the wifi connection
PubSubClient mqttClient(client);

// Channel ID defined in the ThinkSpeak account. Up to eight fields per channel
#define channelID 3150934         // Holds three fields: La, Lo & Time

// ThingSpeak certificate
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

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                                     Functions                                         ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

// Definitions
void gps_init();      // Function to initialize GPS (GT-U7)
void oled_init();     // Function to initialize OLED display
void serial_gps();    // Function to show GPS data in the serial monitor
void display_gps();   // Function to show GPS data in the OLED display
void wifi_connect();  // Function to connect the specified WiFi network

void mqttConnect();                             // Function to connect to MQTT server, i.e., mqtt3.thingspeak.com
void mqttSubscribe(long);                       // Function to subscribe to ThingSpeak channel for updates
void mqttPublish(long, String);                 // Function to publish messages to a ThingSpeak channel
void mqttCallback(char*, byte*, unsigned int);  // Function to handle messages from MQTT subscription to the ThingSpeak broker

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                                       Main                                            ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

void setup() {
  Serial.begin(115200);   // Initialize serial monitor
  gps_init();             // Initialize GPS
  oled_init();            // Initialize the OLED display
  wifi_connect();         // Connect to WiFi

  // Configure the MQTT client to connect with ThingSpeak broker and properly handle messages
  mqttClient.setServer(server, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(2048);

  // Use secure MQTT connections if defined.
  #ifdef USESECUREMQTT
      client.setCACert(thingspeak_ca_cert);
  #endif
}


void loop() {
  // Reconnect to WiFi if it gets disconnected.
  if (WiFi.status() != WL_CONNECTED) wifi_connect();

  // Connect to the MQTT client
  if (!mqttClient.connected()) {
    mqttConnect();
    mqttSubscribe( channelID );
  }

  // Call the loop to maintain connection to the server.
  mqttClient.loop();

  // Main read-publish loop
  if(millis() - lastUpdate > 1000 * updateInterval) {
    // Read data from GPS
    while(SerialGPS.available()) {
      char c = SerialGPS.read();
      gps.encode(c);
    }

    // Store data in the corresponding variables
    gps.f_get_position(&latitude, &longitude);
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);

    // Show GPS data in the serial monitor
    serial_gps();

    // Show GPS data in OLED display
    display_gps();

    // MQTT Publish
    mqttPublish(channelID, (String("field1=")+String(latitude) + String("&field2=")+String(longitude)));

    lastUpdate = millis();
  }
}

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                                     Functions                                         ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

// Initialize GPS
void gps_init() {
  delay(1500);
  SerialGPS.begin(9600, SERIAL_8N1, tx, rx);
  Serial.println("\nInitializing GPS...");
  while(SerialGPS.available()) SerialGPS.read();
  delay(500);
}

// Initialize the OLED display
void oled_init() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
}

// Show GPS data in the serial monitor
void serial_gps() {
  latitude = latitude == COORD_FAILED ? 0 : latitude;
  longitude = longitude == COORD_FAILED ? 0 : longitude;
  sat = gps.satellites() == SAT_FAILED ? 0 : gps.satellites();
  Serial.print("\nLat: "); Serial.println(latitude, 8);
  Serial.print("Lon: "); Serial.println(longitude, 8);
  Serial.print("Satellites: "); Serial.println(sat);
}

// Show GPS data in OLED display
void display_gps() {
  // Clear
  display.clearDisplay();
  display.setTextSize(1);
  // "Latitude"
  display.setCursor(0,0);
  display.print("Latitude: ");
  display.print(latitude);
  display.cp437(true);
  display.write(167);
  // "Longitude"
  display.setCursor(0, 12);
  display.print("Longitude: ");
  display.print(longitude);
  display.cp437(true);
  display.write(167);
  // "Time"
  display.setCursor(0, 24);
  display.print("Time: ");
  if(hour < 6) display.print(hour + 18);
  else if(hour < 16) display.print("0");
  display.print(hour - 6);
  display.print(":");
  if(minute < 10) display.print("0");
  display.print(minute);
  display.print(":");
  if(second < 10) display.print("0");
  display.print(second);

  display.display();
}

// Function to connect to WiFi
void wifi_connect() {
  Serial.println( "Connecting...\n");
  // Loop until WiFi connection is successful
  while(WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(1000*connectionDelay);
    Serial.println(WiFi.status());
  }
  Serial.println("Connected to Wi-Fi.");
}

// Function to connect to MQTT server
void mqttConnect() {
  // Loop until the client is connected to the server
  while (!mqttClient.connected()) {
    // Connect to the MQTT broker
    if(mqttClient.connect(clientID, mqttUserName, mqttPass)) {
      Serial.print( "MQTT to " );
      Serial.print( server );
      Serial.print (" at port ");
      Serial.print( mqttPort );
      Serial.println( " successful." );
    } else {
      Serial.print( "MQTT connection failed, rc = " );
      Serial.print( mqttClient.state() );
      Serial.println( " Will try the connection again in a few seconds" );
      delay( connectionDelay*1000 );
    }
  }
}

// Function to subscribe to ThingSpeak channel for updates
void mqttSubscribe( long subChannelID ) {
  String myTopic = "channels/"+String( subChannelID )+"/subscribe";
  mqttClient.subscribe(myTopic.c_str());
}

// Function to handle messages from MQTT subscription to the ThingSpeak broker
void mqttCallback( char* topic, byte* payload, unsigned int length ) {
  // Print the message details that was received to the serial monitor
  Serial.print("Message arrived from ThinksSpeak broker [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

// Function to publish messages to a ThingSpeak channel
void mqttPublish(long pubChannelID, String message) {
  String topicString ="channels/" + String( pubChannelID ) + "/publish";
  mqttClient.publish( topicString.c_str(), message.c_str() );
}


// GT-U7
// 161213972
// 2506
