# Unified server containing everything this far
from flask import Flask, request, send_file, jsonify, Response
import time, os
from datetime import datetime
import plotly.express as px
import pandas as pd
import numpy as np
import io

app = Flask(__name__)
TESTFILE = "testfile.bin"


# REAL-TIME METRICS STORAGE
current_metrics = {
    "avg_download": 0,
    "avg_upload": 0,
    "num_queries": 0,
    "current_time": "",
}

# Internal rolling tracking
upload_speeds = []      # Mbps
download_speeds = []    # Mbps

#  ROUTE: SERVE TEST FILE (DOWNLOAD SPEED)
@app.route('/testfile.bin', methods=['GET'])
def serve_file():
    start = time.time()
    response = send_file(TESTFILE, mimetype='application/octet-stream', as_attachment=True)
    duration = time.time() - start

    # Calculate download speed
    size = os.path.getsize(TESTFILE)
    mbps = (size * 8 / duration) / 1e6 if duration > 0 else 0
    download_speeds.append(mbps)

    # Update averages
    if download_speeds:
        current_metrics["avg_download"] = round(sum(download_speeds) / len(download_speeds), 2)

    current_metrics["num_queries"] += 1
    return response


#  ROUTE: UPLOAD TEST (UPLOAD SPEED)
@app.route('/upload', methods=['POST'])
def upload():
    start = time.time()
    data = request.get_data()
    duration = time.time() - start
    size = len(data)

    mbps = (size * 8 / duration) / 1e6 if duration > 0 else 0
    upload_speeds.append(mbps)

    # Update averages
    if upload_speeds:
        current_metrics["avg_upload"] = round(sum(upload_speeds) / len(upload_speeds), 2)

    current_metrics["num_queries"] += 1

    return jsonify({
        "size": size,
        "duration_s": duration,
        "mbps": mbps
    }), 200


#  ROUTE: METRICS FOR FRONTEND
@app.route('/metrics', methods=['GET'])
def metrics():
    # Update current time
    current_metrics["current_time"] = datetime.now().strftime("%H:%M:%S")

    return jsonify(current_metrics)


#  ROUTE: LIVE PLOT (AUTOMATICALLY UPDATING)
@app.route("/plot")
def live_plot():
    """
    Returns a Plotly 3D scatter plot that refreshes automatically.
    """

    # Fake dynamic data example (replace with your real df)
    N = 40
    df = pd.DataFrame({
        "latitude": np.random.uniform(-10, 10, N),
        "longitude": np.random.uniform(-10, 10, N),
        "throughput": np.random.uniform(1, 50, N),
    })

    # Color palette gradient (low → high)
    colors = [
        "#A24E35",
        "#BF915A",
        "#C6C2A4",
        "#80949D",
        "#09181A"
    ]

    fig = px.scatter_3d(
        df,
        x="latitude",
        y="longitude",
        z="throughput",
        color="throughput",
        color_continuous_scale=colors,
        opacity=0.9,
        height=600
    )

    # No grid, only axes
    fig.update_scenes(
        xaxis=dict(showgrid=False, zeroline=False, showbackground=False),
        yaxis=dict(showgrid=False, zeroline=False, showbackground=False),
        zaxis=dict(showgrid=False, zeroline=False, showbackground=False),
    )

    fig.update_layout(
        paper_bgcolor="white",
        margin=dict(l=0, r=0, t=0, b=0)
    )

    # Auto-refresh every 2 seconds
    html = f"""
    <html>
      <head>
        <meta http-equiv="refresh" content="2">
      </head>
      <body>
        {fig.to_html(include_plotlyjs='cdn', full_html=False)}
      </body>
    </html>
    """

    return Response(html, mimetype='text/html')


#  MAIN SERVER ENTRY
if __name__ == "__main__":
    # Create test file if missing
    if not os.path.exists(TESTFILE):
        with open(TESTFILE, "wb") as f:
            f.write(b"\0" * 1024 * 1024)   # 1 MiB
        print("Created testfile.bin (1 MiB)")

    print("Running server on http://localhost:8080")
    app.run(host='0.0.0.0', port=8080)
