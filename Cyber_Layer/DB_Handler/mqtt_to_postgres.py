# Mqtt subscriber to handle the data base
# --- Imports ---
import logging
import os
from datetime import datetime
from urllib.parse import parse_qsl

import paho.mqtt.client as mqtt
import psycopg
from psycopg_pool import ConnectionPool
from tenacity import retry, stop_after_attempt, wait_exponential

# --- CONFIG ---
# Mqtt environment variables
MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")         # Location of the Mqtt broker
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))               # Port of the broker
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "channels/+/publish")  # Structure of the many channels
MQTT_QOS = 1
# PgAdmin access variables
PG_HOST = os.getenv("PG_HOST", "localhost")                 # Location of the database
PG_PORT = int(os.getenv("PG_PORT", 5432))                   # Port
PG_DB = os.getenv("PG_DB", "IoT1")                          # Database name
PG_USER = os.getenv("PG_USER", "postgres")                  # Owner of the database
# PG_PASS = os.getenv("PG_PASS", 'vivaHAASf1"')               # Charly's Password
PG_PASS = os.getenv("PG_PASS", "CMF-THW-PNK-79l")           # Franco's Password
# Database connection parameters
PG_MIN_CONN = 1
PG_MAX_CONN = 10

# --- Logging ---
# timestamps
logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")

# --- Psycopg3 Connection Pool ---
pool = None

def init_pg_pool():     # Creates a connection pool, just one
    global pool
    if pool is None:
        logging.info("Creating psycopg3 connection pool...")

        # Creates a PostgreSQL connection pool
        pool = ConnectionPool(
            conninfo=f"host={PG_HOST} port={PG_PORT} dbname={PG_DB} user={PG_USER} password={PG_PASS}",
            min_size=PG_MIN_CONN,
            max_size=PG_MAX_CONN,
            num_workers=1,         # For safe threaded usage with paho mqtt
        )

        logging.info("Pool created OK.")


# --- Insert with retry ---
@retry(stop=stop_after_attempt(5), wait=wait_exponential(min=1, max=10))
def insert_row(data):
    """
    Insert dictionary:
    { device_id, rssi, down, up, lat, lon, ts }
    """

    try:
        # Retrieves a pooled DB connection
        with pool.connection() as conn:
            with conn.cursor() as cur:
                # Inserts one row into the database
                cur.execute(
                    """
                    INSERT INTO network_data
                        (device_id, rssi, down, up, lat, lon, ts)
                    VALUES (%s,%s,%s,%s,%s,%s,%s)
                    """,
                    (
                        data.get("device_id"),
                        data.get("rssi"),
                        data.get("down"),
                        data.get("up"),
                        data.get("lat"),
                        data.get("lon"),
                        data.get("ts") or datetime.utcnow(),    # If timestamp is missing
                    ),
                )
                conn.commit()       # Commits transaction
        # Logs succesful insertion
        logging.info("Inserted: %s @ %s", data.get("device_id"), data.get("ts"))

    except Exception as e:
        # Logs error and tries again, exponentially up to 5 times
        logging.exception("Insert failed, will retry: %s", e)
        raise



# --- Payload parsing ---
def parse_payload(payload_str):
    """
    Parses MQTT payload like:
    field1=rssi&field2=down&...&field7=ts? or field7=unix
    """
    pairs = dict(parse_qsl(payload_str))    # Decodes URL-encoded key/value pairs

    # Buidls the resulting dictionary with a value per field, parses to float
    result = {
        "rssi": float(pairs["field1"]) if "field1" in pairs else None,
        "down": float(pairs["field2"]) if "field2" in pairs else None,
        "up":   float(pairs["field3"]) if "field3" in pairs else None,
        "lat":  float(pairs["field4"]) if "field4" in pairs else None,
        "lon":  float(pairs["field5"]) if "field5" in pairs else None,
        "ts": None,
        "device_id": pairs.get("field7"),
    }

    # parse timestamp from field6
    if "field6" in pairs:
        raw = pairs["field6"]

        # Try ISO
        try:
            result["ts"] = datetime.fromisoformat(raw.replace("Z", "+00:00"))
        except:
            pass

        # Try UNIX timestamp
        if result["ts"] is None:
            try:
                result["ts"] = datetime.utcfromtimestamp(float(raw))
            except:
                pass

    # Returns the dictionary
    return result


# --- MQTT callbacks ---
# Connects to the mqtt broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logging.info("MQTT connected OK. Subscribing to %s", MQTT_TOPIC)
        client.subscribe(MQTT_TOPIC, qos=MQTT_QOS)
    else:
        logging.error("MQTT connection error rc=%s", rc)

# Decodes Mqtt message, logs the message
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8", errors="ignore")
        logging.info("MQTT msg | topic=%s | payload=%s", msg.topic, payload)

        parsed = parse_payload(payload)     # Parses payload into the dictionary
        if parsed:
            insert_row(parsed)              # Inserts into the database
        else:
            logging.warning("Could not parse payload: %s", payload) # Logs error if parsing fails

    # Catches errors
    except Exception as e:
        logging.exception("Error in MQTT message handling: %s", e)


# --- MAIN ---
def main():
    init_pg_pool()  # Initializes PostgreSQL connection pool

    client = mqtt.Client(client_id="mqtt_to_pg_psycopg3")   # Creates MQTT client

    #Registers callback functions
    logging.info("Connecting to MQTT broker %s:%d ...", MQTT_BROKER, MQTT_PORT)
    client.on_connect = on_connect
    client.on_message = on_message

    # Starts the MQTT connection
    client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)

    try:
        client.loop_forever()   # Enters MQTT loop handling incoming traffic
    except KeyboardInterrupt:   # Shutdown with Control + C
        logging.info("Exiting...")


if __name__ == "__main__":      # Only runs as a script
    main()
