/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢ 1) ESP32 reads location and timestamp fron GT-U7 through the digital pins 16 & 17    ▢
 ▢ 2) ESP32 sends data to the built-in serial port to the host computer (115200)        ▢
 ▢ 3) ESP32 sends the location reading to an attached OLED display (SSD1306) if         ▢
 ▢    available, through the I²C bus: (SDA --> GP21, SCL --> GP22)                      ▢
 ▢ 4) The read data is packed into a MQTT package and send to the broker (ThingSpeak)   ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                                      Libraries                                       ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <TinyGPSPlus.h>
#include <TimeLib.h>
#include <math.h>
#include <WiFi.h>

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                               Variables and Definitions                              ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

// Set up the GPS sensor and its variables
TinyGPSPlus gps;                    // GT-U7 GPS definition
HardwareSerial SerialGPS(1);        // Create new serial port for GPS
float latitude{0}, longitude{0};    // Variables for storing the coordinates
int sat{0}, rx{16}, tx{17};         // Physical serial ports used and satellites

// Spacial reference variables
const double EARTH_RADIUS = 6371000.0;                  // Earth radius in meters
const double ONE_DEG = EARTH_RADIUS * 2 * M_PI / 360;   // One rotation degree in meters
const double REF_LAT = 19.01620;    // Reference latitude in degrees
const double REF_LON = -98.24581;   // Reference longitude in degrees

// Variables for storing time
int yy{0}, mm{0}, dd{0}, hs{0}, mins{0}, secs{0};

// Set up the SSD1306 display connected to I2C (SDA, SCL)
#define SCREEN_WIDTH 128            // OLED display width  (pixels)
#define SCREEN_HEIGHT 32            // OLED display height (pixels)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor printing and publishing intervals
int delayPub{10}, delayPrint{3};    // Sensor readings are published every 10 seconds and printed every 3
long lastPub{0}, lastPrint{0};      // To hold the value of last call of the millis() function

// WiFi set-up variables and credentials
const int connectionDelay = 3;                  // Delay between attemps
const char* ssid = "Tec-IoT";                   // Network name
const char* pass = "spotless.magnetic.bridge";  // Network password

// Channel ID defined in the ThinkSpeak account. Up to eight fields per channel
#define channelID 3150934           // Holds three fields: Lat, Lon & Time
#define mqttPort 1883
WiFiClient client;

// ThinkSpeak credentials – account and the defined channels
const char* mqttUserName = "FSIPNTozNBIMIzMSDAUfDj0";
const char* clientID     = "FSIPNTozNBIMIzMSDAUfDj0";
const char* mqttPass     = "AxH7xNM6mYufHIcV8B59V7gN";

// MQTT broker server
const char* server = "mqtt3.thingspeak.com";
PubSubClient mqttClient(client);

/*
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 ▢                                     Functions                                        ▢
 ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢ ▢
 */

 // Definitions
 void gps_read();                                   // Function to read location from the GPS
 void gps_init();                                   // Function to initialize GPS (GT-U7)
 void oled_init();                                  // Function to initialize OLED display
 void get_meters();                                 // Function to convert latitude and longitude to meters
 void serial_gps();                                 // Function to show GPS data in the serial monitor
 void display_gps();                                // Function to show GPS data in the OLED display
 void wifi_connect();                               // Function to connect the specified WiFi network

 void mqtt_init();                                   // Function to configure the MQTT client to connect with ThingSpeak and handle messages
 void mqtt_connect();                                // Function to connect to MQTT server, i.e., mqtt3.thingspeak.com
 void mqtt_publish(long);                            // Function to publish messages to a ThingSpeak channel
 void mqtt_subscribe(long);                          // Function to subscribe to ThingSpeak channel for updates
 void mqtt_callback(char*, byte*, unsigned int);     // Function to handle messages from MQTT subscription to the ThingSpeak broker


// Read GPS
void gps_read() {
    do {
        // Read data from GPS
        while(SerialGPS.available()) {
            char c = SerialGPS.read();
            gps.encode(c);
        }

        // Store data
        sat = gps.satellites.value();
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        yy = gps.date.year();
        mm = gps.date.month();
        dd = gps.date.day();
        hs = gps.time.hour();
        mins = gps.time.minute();
        secs = gps.time.second();

        // Check connection
        if(!gps.location.isValid()) {
        Serial.print("GPS has no signal\n");
        delay(connectionDelay * 1000);
        }

    // Repeat while not connected
    } while(!gps.location.isValid());

    // Convert to meters
    get_meters();
}

// Initialize GPS
void gps_init() {
    delay(1500);
    Serial.println("\nInitializing GPS...");
    SerialGPS.begin(9600, SERIAL_8N1, tx, rx);
    while(SerialGPS.available()) SerialGPS.read();
    delay(500);
}

// Initialize the OLED display
void oled_init() {
    // Check status
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }

    // Reinitialize display
    delay(2000);
    display.clearDisplay();
    display.setTextColor(WHITE);
}

// Show GPS data in the serial monitor
void serial_gps() {
    if(millis() - lastPrint > 1000 * delayPrint) {
        lastPrint = millis();
        Serial.print("\nLat: "); Serial.println(latitude, 5);
        Serial.print("Lon: "); Serial.println(longitude, 5);
        Serial.print("Satellites: "); Serial.println(sat);
    }
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
    display.print(" m");

    // "Longitude"
    display.setCursor(0, 12);
    display.print("Longitude: ");
    display.print(longitude);
    display.print(" m");

    // "Time"
    display.setCursor(0, 24);
    display.print("Time: ");

    // Hour
    if(hs < 6) display.print(hs + 18);
    else {
        if(hs < 16) display.print("0");
        display.print(hs - 6);
    }
    display.print(":");

    // Minute
    if(mins < 10) display.print("0");
    display.print(mins);
    display.print(":");

    // Second
    if(secs < 10) display.print("0");
    display.print(secs);

    display.display();
}

// Function to connect to WiFi
void wifi_connect() {
    // Check connection
    if(WiFi.status() == WL_CONNECTED) return;

    // Reconnect
    Serial.println( "Connecting...\n");
    while(WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, pass);
        delay(connectionDelay * 1000);
        Serial.println(WiFi.status());
    }
    Serial.println("Connected to Wi-Fi.");
}

// Function to configure the MQTT client to connect with ThingSpeak and handle messages
void mqtt_init() {
    mqttClient.setServer(server, mqttPort);
    mqttClient.setCallback(mqtt_callback);
    mqttClient.setBufferSize(2048);
}

// Function to connect to MQTT server
void mqtt_connect() {
  // Loop until the client is connected to the server
  while (!mqttClient.connected()) {

    // Connection succeed
    if(mqttClient.connect(clientID, mqttUserName, mqttPass)) {
      Serial.print("MQTT to ");
      Serial.print(server);
      Serial.print (" at port ");
      Serial.print(mqttPort);
      Serial.println(" successful.");

    // Connection fail
    } else {
      Serial.print("MQTT connection failed, rc = ");
      Serial.print(mqttClient.state());
      Serial.println(" Will try the connection again in a few seconds");
      delay(connectionDelay * 1000);
    }
  }
  // Subscribe channel
  mqtt_subscribe(channelID);
}

// Function to subscribe to ThingSpeak channel for updates
void mqtt_subscribe(long subChannelID) {
  String myTopic = "channels/" + String(subChannelID) + "/subscribe";
  mqttClient.subscribe(myTopic.c_str());
}

// Function to handle messages from MQTT subscription to the ThingSpeak broker
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // Print the message details that was received to the serial monitor
  Serial.print("Message arrived from ThinksSpeak broker [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

// Function to make the string that is passed as the MGTT prompt
void mqtt_publish(long pubChannelID) {
    if(millis() - lastPub > 1000 * delayPub) {
        lastPub = millis();

        // Set the time using the GPS variables and adjust to UTC-6
        setTime(hs, mins, secs, dd, mm, yy);
        time_t date = now() - 6 * 3600;

        // Create a query with the MQTT prompt
        String query{""};
        query += String("field1=") + String(latitude);
        query += String("&field2=") + String(longitude);
        query += String("&field3=") + String(date);

        // Create a string with channel ID
        String topicString ="channels/" + String(pubChannelID) + "/publish";

        // Publish data
        mqttClient.publish(topicString.c_str(), query.c_str());
    }
}

// Function to convert latitude and longitude into meters
void get_meters() {
    // Conversion constants
    const double m_per_deg_lat = ONE_DEG;                                  // meters per degree latitude
    const double m_per_deg_lon = ONE_DEG * cos(REF_LAT * (M_PI / 180.0));  // meters per degree longitude at refLat

    // Differences in degrees
    double dLat = latitude  - REF_LAT;
    double dLon = longitude - REF_LON;

    // Conversion to meters
    double north_m = dLat * m_per_deg_lat;
    double east_m  = dLon * m_per_deg_lon;

    // Overwrite global variables
    latitude  = (float)north_m;
    longitude = (float)east_m;
}
