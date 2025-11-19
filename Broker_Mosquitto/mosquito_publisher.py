# publisher.py
import paho.mqtt.client as mqtt
import time

broker = "localhost"  # O IP del broker si es remoto
port = 1883

client = mqtt.Client()
client.connect(broker, port)

while True:
    client.publish("topic/timestamp", "2025-11-10T12:00:00")
    client.publish("topic/throughput", "-70")  # ejemplo de RSSI
    client.publish("topic/coordinates", "Lat:19.4,Lon:-99.1")
    print("✅ Publicado")
    time.sleep(5)  # publica cada 5 segundos
