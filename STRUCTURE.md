src/
├—— Publisher_Node/
│   ├—— config.h
│   ├—— main.h
│   └─— main.cpp
├—— Consumer_Node/
│   ├—— config.h
│   ├—— main.h
│   └─— main.cpp
├—— Cyber_Layer/
│   ├—— Broker/
│   │   └—— mosquitto.conf
│   ├—— DB_Handler/
│   │   └—— mqtt_to_postgres.py
│   └─— Local_Server/
│       └—— combined_server.py
├—— Analytics_Layer/
│   └—— AnalysisLayer.py
└—— Application_Layer/
    ├—— IoT_Frontend/
    │   └—— ...
    └—— package-lock.json

// Steps:
  1. Start postgreSQL
    brew services start postgresql
  2. Start mosquitto
    brew services start mosquitto
  3. Start python servers
    ~python mqtt_to_postgres
    ~python server.py
  4. Launch web page
    ~npm run dev
