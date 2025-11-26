# Cyber Layer
This folder contains the components that receive MQTT messages from the ESP32, process them, and store the data in PostgreSQL.

### 1. Broker Configuration
- Configuration files for the Mosquitto MQTT broker.

### 2. Data Base Handler
- Python script that subscribes to the MQTT broker, parses incoming ESP32 messages, and inserts them into the PostgreSQL database.

### 3. Local Server
- Simple HTTP server that handles upload and download tests and exposes endpoints used by the ESP32 for throughput measurement.
