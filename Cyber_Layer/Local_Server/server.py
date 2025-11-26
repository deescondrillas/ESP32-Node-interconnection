# Unified server containing everything so far
from flask import Flask, request, send_file, jsonify, Response
from datetime import datetime
from flask_cors import CORS
import plotly.express as px
import plotly.io as pio
import pandas as pd
import numpy as np
import time, os
import psycopg


# CONFIG
app = Flask(__name__)
CORS(app)
TESTFILE = "testfile.bin"

# PostgreSQL connection info
PG_HOST = "localhost"
PG_PORT = 5432
PG_DB   = "IoT1"
PG_USER = "postgres"
# PG_PASS = 'vivaHAASf1"'         # Charly's Password
PG_PASS = "CMF-THW-PNK-79l"     # Franco's Password
DEVICE_ID = "ESP32_00"

# REAL-TIME METRICS STORAGE
current_metrics = {
    "avg_download": 0,
    "avg_upload": 0,
    "num_queries": 0,
    "current_time": "",
}

# Internal rolling tracking (in-memory)
upload_speeds = []      # Mbps
download_speeds = []    # Mbps


# DATABASE
def get_db_connection():
    return psycopg.connect(
        host=PG_HOST,
        port=PG_PORT,
        dbname=PG_DB,
        user=PG_USER,
        password=PG_PASS
    )

# Ensure table exists
def init_db():
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS network_data (
            device_id TEXT NOT NULL,
            rssi DOUBLE PRECISION,
            down DOUBLE PRECISION,
            up DOUBLE PRECISION,
            lat DOUBLE PRECISION,
            lon DOUBLE PRECISION,
            ts TIMESTAMP NOT NULL,
            PRIMARY KEY (device_id, ts)
        )
    """)
    conn.commit()
    cur.close()
    conn.close()


# ROUTES

# Serve test file (download speed)
@app.route('/testfile.bin', methods=['GET'])
def serve_file():
    start = time.time()
    response = send_file(TESTFILE, mimetype='application/octet-stream', as_attachment=True)
    duration = time.time() - start

    # Calculate download speed in Mbps
    size = os.path.getsize(TESTFILE)
    mbps = (size * 8 / duration) / 1e6 if duration > 0 else 0
    download_speeds.append(mbps)

    # Update in-memory averages
    if download_speeds:
        current_metrics["avg_download"] = round(sum(download_speeds) / len(download_speeds), 2)
    current_metrics["num_queries"] += 1

    # Insert into DB
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute(
        "INSERT INTO network_data (device_id, rssi, down, up, lat, lon, ts) VALUES (%s,%s,%s,%s,%s,%s,%s)",
        (DEVICE_ID, None, mbps, None, None, None, datetime.now())
    )
    conn.commit()
    cur.close()
    conn.close()

    return response

# Upload route (upload speed)
@app.route('/upload', methods=['POST'])
def upload():
    start = time.time()
    data = request.get_data()
    duration = time.time() - start
    size = len(data)

    mbps = (size * 8 / duration) / 1e6 if duration > 0 else 0
    upload_speeds.append(mbps)

    # Update in-memory averages
    if upload_speeds:
        current_metrics["avg_upload"] = round(sum(upload_speeds) / len(upload_speeds), 2)
    current_metrics["num_queries"] += 1

    # Insert into DB
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute(
        "INSERT INTO network_data (device_id, rssi, down, up, lat, lon, ts) VALUES (%s,%s,%s,%s,%s,%s,%s)",
        (DEVICE_ID, None, None, mbps, None, None, datetime.now())
    )
    conn.commit()
    cur.close()
    conn.close()

    return jsonify({
        "size": size,
        "duration_s": duration,
        "mbps": mbps
    }), 200

# Metrics route (aggregated from DB, all devices)
@app.route('/metrics', methods=['GET'])
def metrics():
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute("""
        SELECT
            COALESCE(AVG(down), 0) AS avg_download,
            COALESCE(AVG(up), 0) AS avg_upload,
            COUNT(*) AS num_queries,
            MAX(ts) AS last_ts
        FROM network_data
    """)
    row = cur.fetchone()
    cur.close()
    conn.close()

    return jsonify({
        "avg_download": float(row[0]),
        "avg_upload": float(row[1]),
        "num_queries": row[2],
        "current_time": datetime.now().strftime("%H:%M:%S")  # always current time
    })


# Live plot (Plotly 3D) with real data
@app.route("/plot")
def live_plot():
    conn = get_db_connection()
    cur = conn.cursor()

    # Query last N points per device (or all)
    cur.execute("""
        SELECT device_id, lat, lon, down, up, ts
        FROM network_data
        WHERE lat IS NOT NULL AND lon IS NOT NULL
        ORDER BY ts DESC
        LIMIT 100
    """)

    rows = cur.fetchall()
    cur.close()
    conn.close()

    if not rows:
        return "<h3>No data available for plotting.</h3>"

    # Create DataFrame
    df = pd.DataFrame(rows, columns=["device_id", "lat", "lon", "down", "up", "ts"])

    # Choose which throughput to plot (e.g., download)
    df["throughput"] = df["down"].fillna(0)

    colors = ["#A24E35", "#BF915A", "#C6C2A4", "#80949D", "#09181A"]

    fig = px.scatter_3d(
        df,
        x="lat",
        y="lon",
        z="throughput",
        color="throughput",
        color_continuous_scale=colors,
        opacity=0.9,
        height=580,
        hover_data=["device_id", "down", "up", "ts"]
    )

    fig.update_scenes(
        xaxis=dict(title="Latitude (m)", showgrid=False, zeroline=False, showbackground=False),
        yaxis=dict(title="Longitude (m)", showgrid=False, zeroline=False, showbackground=False),
        zaxis=dict(title="Download speed (MBps)", showgrid=False, zeroline=False, showbackground=False)
    )

    fig.update_layout(coloraxis_colorbar=dict(title="MBps"))
    fig.update_layout(paper_bgcolor="white", margin=dict(l=0, r=0, t=0, b=0))
    fig.update_layout(autosize=True, margin=dict(l=0, r=0, t=0, b=0))


    # Auto-refresh every second
    html = f"""
    <html>
      <body>
        {fig.to_html(include_plotlyjs='cdn', full_html=False)}
      </body>
    </html>
    """
    return Response(html, mimetype='text/html')


# Convert to JPEG
@app.route("/plot.jpg")
def live_plot_jpeg():
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute("""
        SELECT device_id, lat, lon, down, up, ts
        FROM network_data
        WHERE lat IS NOT NULL AND lon IS NOT NULL
        ORDER BY ts DESC
        LIMIT 100
    """)
    rows = cur.fetchall()
    cur.close()
    conn.close()

    if not rows:
        return "<h3>No data available.</h3>", 404

    df = pd.DataFrame(rows, columns=["device_id", "lat", "lon", "down", "up", "ts"])
    df["throughput"] = df["down"].fillna(0)

    fig = px.scatter_3d(
        df,
        x="lat",
        y="lon",
        z="throughput",
        color="throughput",
        color_continuous_scale=["#A24E35","#BF915A","#C6C2A4","#80949D","#09181A"],
        opacity=0.9,
        height=160,
        width=128,
        hover_data=["device_id","down","up","ts"]
    )

    # Convert to JPEG
    img_bytes = pio.to_image(fig, format="jpeg", width=128, height=160, scale=1)
    return Response(img_bytes, mimetype="image/jpeg")

# MAIN SERVER ENTRY
if __name__ == "__main__":
    # Create test file if missing
    if not os.path.exists(TESTFILE):
        with open(TESTFILE, "wb") as f:
            f.write(b"\0" * 1024 * 1024)  # 1 MiB
        print("Created testfile.bin (1 MiB)")

    # Initialize DB table
    init_db()

    print("Running server on http://localhost:8080")
    app.run(host='0.0.0.0', port=8080)
