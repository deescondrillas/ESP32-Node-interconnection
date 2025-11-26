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


// npm install --legacy-peer-deps
// (npm i)
// npm run dev
