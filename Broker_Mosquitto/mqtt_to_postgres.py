import os, json, time, signal, sys
from datetime import datetime, timezone
from dotenv import load_dotenv
import paho.mqtt.client as mqtt
import psycopg2
from psycopg2.pool import SimpleConnectionPool

# --------- Carga de configuración ---------
load_dotenv()

MQTT_HOST = os.getenv("MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_USERNAME = os.getenv("MQTT_USERNAME") or None
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD") or None
MQTT_TOPICS = [t.strip() for t in os.getenv("MQTT_TOPICS", "esp32/#,arduino/#").split(",") if t.strip()]

PGHOST = os.getenv("PGHOST", "localhost")
PGPORT = int(os.getenv("PGPORT", "5432"))
PGDATABASE = os.getenv("PGDATABASE", "iot_data")
PGUSER = os.getenv("PGUSER", "postgres")
PGPASSWORD = os.getenv("PGPASSWORD", "")

# --------- Pool de PostgreSQL ---------
pg_pool = SimpleConnectionPool(
    1, 10,
    host=PGHOST, port=PGPORT, dbname=PGDATABASE,
    user=PGUSER, password=PGPASSWORD
)

DDL_CREATE = """
CREATE TABLE IF NOT EXISTS devices (
    id SERIAL PRIMARY KEY,
    device_key TEXT UNIQUE,
    first_seen TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_seen  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS readings (
    id BIGSERIAL PRIMARY KEY,
    device_id INTEGER REFERENCES devices(id) ON DELETE SET NULL,
    topic TEXT NOT NULL,
    payload JSONB NOT NULL,
    ts TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Tabla “plana” opcional para métricas comunes (si se detectan):
CREATE TABLE IF NOT EXISTS readings_flat (
    id BIGSERIAL PRIMARY KEY,
    device_id INTEGER REFERENCES devices(id) ON DELETE SET NULL,
    topic TEXT NOT NULL,
    ts TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    rssi INT,
    temperature DOUBLE PRECISION,
    humidity DOUBLE PRECISION,
    pressure DOUBLE PRECISION,
    voltage DOUBLE PRECISION,
    current DOUBLE PRECISION,
    pm25 DOUBLE PRECISION,
    pm10 DOUBLE PRECISION
);

CREATE INDEX IF NOT EXISTS idx_readings_ts ON readings(ts);
CREATE INDEX IF NOT EXISTS idx_readings_topic ON readings(topic);
CREATE INDEX IF NOT EXISTS idx_readings_payload_gin ON readings USING GIN (payload);
CREATE INDEX IF NOT EXISTS idx_flat_ts ON readings_flat(ts);
"""

def ensure_schema():
    conn = pg_pool.getconn()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(DDL_CREATE)
    finally:
        pg_pool.putconn(conn)

ensure_schema()

# --------- Utilidades de DB ---------
def upsert_device(device_key:str) -> int:
    conn = pg_pool.getconn()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute("""
                    INSERT INTO devices(device_key) VALUES (%s)
                    ON CONFLICT (device_key) DO UPDATE SET last_seen = NOW()
                    RETURNING id;
                """, (device_key,))
                (device_id,) = cur.fetchone()
                return device_id
    finally:
        pg_pool.putconn(conn)

def insert_reading(device_id:int, topic:str, payload:dict):
    conn = pg_pool.getconn()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute("""
                    INSERT INTO readings(device_id, topic, payload, ts)
                    VALUES (%s, %s, %s, NOW());
                """, (device_id, topic, json.dumps(payload)))
    finally:
        pg_pool.putconn(conn)

COMMON_KEYS = {"rssi","temperature","humidity","pressure","voltage","current","pm25","pm10"}

def insert_flat_if_possible(device_id:int, topic:str, payload:dict):
    # Solo inserta si encuentra al menos una clave común
    flat = {k: payload.get(k) for k in COMMON_KEYS if k in payload}
    # Casteos seguros
    def as_float(x):
        try: return float(x)
        except: return None
    def as_int(x):
        try: return int(float(x))
        except: return None

    mapped = {
        "rssi": as_int(flat.get("rssi")),
        "temperature": as_float(flat.get("temperature")),
        "humidity": as_float(flat.get("humidity")),
        "pressure": as_float(flat.get("pressure")),
        "voltage": as_float(flat.get("voltage")),
        "current": as_float(flat.get("current")),
        "pm25": as_float(flat.get("pm25")),
        "pm10": as_float(flat.get("pm10")),
    }
    if not any(v is not None for v in mapped.values()):
        return

    conn = pg_pool.getconn()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute("""
                    INSERT INTO readings_flat(
                        device_id, topic, ts,
                        rssi, temperature, humidity, pressure, voltage, current, pm25, pm10
                    ) VALUES (%s,%s,NOW(),%s,%s,%s,%s,%s,%s,%s,%s)
                """, (
                    device_id, topic,
                    mapped["rssi"], mapped["temperature"], mapped["humidity"],
                    mapped["pressure"], mapped["voltage"], mapped["current"],
                    mapped["pm25"], mapped["pm10"]
                ))
    finally:
        pg_pool.putconn(conn)

# --------- Parseo de payloads ---------
def try_parse_payload(raw:bytes) -> dict:
    text = raw.decode(errors="ignore").strip()

    # 1) JSON directo
    try:
        obj = json.loads(text)
        if isinstance(obj, dict):
            return obj
        # Si es lista JSON, la empaquetamos
        if isinstance(obj, list):
            return {"values": obj}
    except json.JSONDecodeError:
        pass

    # 2) key=value [; , o espacio] p.ej: "rssi=-65 temp=24.1 humidity=50"
    pairs = {}
    separators = [",", ";", " "]
    tokens = [text]
    for sep in separators:
        tokens = [t for tok in tokens for t in tok.split(sep)]
    eq_tokens = [t for t in tokens if "=" in t]
    if eq_tokens:
        for t in eq_tokens:
            k, v = t.split("=", 1)
            k = k.strip().lower()
            v = v.strip()
            # intento de casteo numérico
            if v.replace(".","",1).replace("-","",1).isdigit():
                try:
                    v = float(v) if "." in v else int(v)
                except:
                    pass
            pairs[k] = v
        if pairs:
            return pairs

    # 3) CSV simple "24.1,50,-65" -> lo guardamos como lista
    if "," in text and all(len(p.strip())>0 for p in text.split(",")):
        try:
            vals = [float(p) if "." in p else int(p) for p in text.split(",")]
            return {"values": vals}
        except:
            return {"raw": text}

    # 4) fallback
    return {"raw": text}

# --------- MQTT ---------
client = mqtt.Client(client_id=f"pg_bridge_{int(time.time())}", clean_session=True)
if MQTT_USERNAME:
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] Conectado ({rc}). Subscribiendo tópicos…")
    for t in MQTT_TOPICS:
        client.subscribe(t, qos=1)
        print(f"   - {t}")

def guess_device_key(topic:str, payload:dict) -> str:
    # Prioriza un device_id que venga en el JSON
    if isinstance(payload, dict):
        for k in ("device_id","device","id","mac","chipid","node"):
            if k in payload and str(payload[k]).strip():
                return str(payload[k]).strip()
    # Si no, deriva del tópico: ej. esp32/lab/telemetry -> esp32/lab
    parts = topic.split("/")
    return "/".join(parts[:2]) if len(parts)>=2 else topic

def on_message(client, userdata, msg):
    try:
        payload = try_parse_payload(msg.payload)
        device_key = guess_device_key(msg.topic, payload)
        device_id = upsert_device(device_key)

        # Enriquecemos con timestamp si no vino
        if isinstance(payload, dict) and "ts" not in payload:
            payload["ts"] = datetime.now(timezone.utc).isoformat()

        insert_reading(device_id, msg.topic, payload)
        insert_flat_if_possible(device_id, msg.topic, payload)

        print(f"[OK] {msg.topic} → dev={device_key} payload={payload}")
    except Exception as e:
        print(f"[ERROR] {e}")

def on_disconnect(client, userdata, rc):
    print(f"[MQTT] Desconectado (rc={rc}). Reintentando…")

client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect

def shutdown(*_):
    print("Cerrando…")
    try:
        client.loop_stop()
        client.disconnect()
    except:
        pass
    try:
        pg_pool.closeall()
    except:
        pass
    sys.exit(0)

signal.signal(signal.SIGINT, shutdown)
signal.signal(signal.SIGTERM, shutdown)

# Conexión y loop
client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
client.loop_forever(retry_first_connection=True)
