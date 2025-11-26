# Analysis Layer.py — Modular version for FastAPI
# --- imports ---
from __future__ import annotations

import os
from typing import Dict, Optional, Tuple

import numpy as np
import pandas as pd
import plotly.graph_objects as go

# import psycopg
from scipy.interpolate import griddata

# --- Zone Delimitation ---
ZONES: Dict[str, Dict[str, float]] = {
    "cafeteria": {"xmin": 178.0, "xmax": 190.0, "ymin": 476.0, "ymax": 486.0},
    "library": {"xmin": 170.0, "xmax": 180.0, "ymin": 460.0, "ymax": 475.0},
    "campus": {"xmin": 1000.0, "xmax": 1000.0, "ymin": 0.0, "ymax": 0.0},
}

# --- PostgreSQL ---
DB_HOST = os.getenv("PGHOST", "localhost")  # DB Location - Local
DB_PORT = os.getenv("PGPORT", "5432")  # Port
DB_NAME = os.getenv("PGDATABASE", "IoT1")  # DB name
DB_USER = os.getenv("PGUSER", "postgres")  # Db's owner
DB_PASS = os.getenv("PGPASSWORD", 'vivaHAASf1"')  # Password


# --- Conection, download and preProcessing ---
# generate synthetic data
def syn_data(n: int = 200) -> pd.DataFrame:
    from datetime import datetime, timedelta

    # Generate coordinates around a campus-like area (meters offsets)
    lat = np.random.uniform(0, 190, n)  # x-range
    lon = np.random.uniform(0, 490, n)  # y-range

    # Generate realistic down/up/rssi values
    down = np.random.uniform(0.02, 1.5, n)  # Mbps
    up = np.random.uniform(0.01, 0.8, n)  # Mbps
    rssi = np.random.uniform(-85, -40, n)  # dBm

    # Generate timestamps within last 24 hours
    now = datetime.now()
    ts = [now - timedelta(minutes=np.random.randint(0, 1440)) for _ in range(n)]

    df = pd.DataFrame(
        {
            "device_id": np.random.randint(1, 5, n),
            "lat": lat,
            "lon": lon,
            "down": down,
            "up": up,
            "rssi": rssi,
            "ts": ts,
        }
    )

    return df


# DB download into a pandas table
def load_data(hours: int = 24) -> pd.DataFrame:
    # SQL query
    q = f"""
    SELECT device_id, rssi, down, up, lat, lon, ts
    FROM network_data
    WHERE ts >= now() - interval '{hours} hours'
    ORDER BY ts
    """

    # Connection
    df = syn_data()

    # Loads into a pandas DataFrame (df)
    df["ts"] = pd.to_datetime(df["ts"])
    # Converts numeric fields to floats
    for col in ["rssi", "down", "up", "lat", "lon"]:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")

    df = df.dropna(subset=["lat", "lon"])  # Drops rows without coords
    return df


# --- Filter ---
def filter_by_zone(df: pd.DataFrame, zone: str) -> pd.DataFrame:
    # Checks existing zone
    if zone not in ZONES:
        raise ValueError(f"Unknown zone: {zone}")
    z = ZONES[zone]

    # Filters data by zone
    return df[
        (df["lat"].between(z["xmin"], z["xmax"]))
        & (df["lon"].between(z["ymin"], z["ymax"]))
    ]


# --- Hourly statistics ---
# Returns a DataFrame with average metric per hour.
def compute_hourly_stats(df: pd.DataFrame, metric: str = "down") -> pd.DataFrame:
    # Asks for a metric to filetr by
    if metric not in ["down", "up", "rssi"]:
        raise ValueError("metric must be 'down', 'up', or 'rssi'")

    df = df.copy()
    df["hour"] = df["ts"].dt.hour

    # Gets the average for that metric
    return df.groupby("hour")[metric].mean().reset_index().sort_values("hour")


# --- Plot ---
# Returns an HTML string containing a 3D surface plot.
# FastAPI can directly return this as HTMLResponse.
def generate_plot(df: pd.DataFrame, metric: str = "down") -> str:
    if df.empty:
        return "<h1>No data to show</h1>"

    if metric not in ["down", "up", "rssi"]:
        raise ValueError("metric must be 'down', 'up', or 'rssi'")

    # Extract values
    x = df["lat"].values
    y = df["lon"].values
    z = df[metric].values

    # Colorscale
    colorscale = [
        [0.0, "#A24E35"],
        [0.25, "#BF915A"],
        [0.50, "#C6C2A4"],
        [0.75, "#80949D"],
        [1.0, "#09181A"],
    ]

    fig = go.Figure(
        data=[
            go.Scatter3d(
                x=x,
                y=y,
                z=z,
                mode="markers",
                marker=dict(
                    size=6,
                    color=z,
                    colorscale=colorscale,
                    showscale=True,
                    colorbar=dict(title="MBps"),
                    opacity=0.95,
                ),
            )
        ]
    )

    fig.update_layout(
        title=dict(
            text=f"<b>3D Network Scatter – {'Download speed'}</b>",
            x=0.5,
            xanchor="center",
            font=dict(size=22),
        ),
        scene=dict(
            xaxis=dict(
                title="Latitude (m)",
                backgroundcolor="#F0F0F0",
                gridcolor="#A0A0A0",
            ),
            yaxis=dict(
                title="Longitude (m)",
                backgroundcolor="#F0F0F0",
                gridcolor="#A0A0A0",
            ),
            zaxis=dict(
                title="Download speed (MBps)",
                backgroundcolor="#F0F0F0",
                gridcolor="#A0A0A0",
            ),
            aspectratio=dict(x=1, y=1, z=0.5),
        ),
        margin=dict(l=20, r=20, b=20, t=60),
        # --- Camera pointing toward the (0,0,0) side ---
        scene_camera=dict(eye=dict(x=-2, y=-2, z=1.5)),
    )

    return fig.to_html(full_html=False, include_plotlyjs="cdn")


# --- heat map ---
def generate_heatmap(df: pd.DataFrame, metric: str = "rssi") -> str:
    if df.empty:
        return "<h1>No data</h1>"

    x = df["lat"].values
    y = df["lon"].values
    z = df[metric].values

    xi = np.linspace(min(x), max(x), 100)
    yi = np.linspace(min(y), max(y), 100)
    Xi, Yi = np.meshgrid(xi, yi)

    Zi = griddata((x, y), z, (Xi, Yi), method="linear")

    fig = go.Figure(data=go.Heatmap(x=xi, y=yi, z=Zi))
    fig.update_layout(title=f"Heatmap: {metric}")

    return fig.to_html(full_html=False, include_plotlyjs="cdn")


# Export to HTML
df = syn_data()

with open("plot.html", "w") as f:
    f.write(generate_plot(df, metric="down"))
