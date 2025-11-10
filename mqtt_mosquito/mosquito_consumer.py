# consumer.py
import paho.mqtt.client as mqtt

def on_message(client, userdata, msg):
    print(f"📥 Mensaje en {msg.topic}: {msg.payload.decode()}")

client = mqtt.Client()
client.connect("localhost", 1883)

# Suscribirse a múltiples tópicos
client.subscribe("topic/timestamp")
client.subscribe("topic/throughput")
client.subscribe("topic/coordinates")

client.on_message = on_message

print("🔄 Esperando mensajes...")
client.loop_forever()
