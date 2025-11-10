import paho.mqtt.client as mqtt
from datetime import datetime

def on_message(client, userdata, msg):
    timestamp = datetime.now().isoformat()
    rssi = msg.payload.decode()
    print(f"[{timestamp}] RSSI: {rssi}")
    # Aquí puedes guardar en una base de datos o archivo CSV

client = mqtt.Client()
client.connect("localhost", 1883)

client.subscribe("topic/timestamp")  # o el nombre que uses
client.on_message = on_message

client.loop_forever()
