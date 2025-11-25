# Analysis Layer.py — Modular version for FastAPI
# --- imports ---
from __future__ import annotations
import os
import psycopg
import pandas as pd
import numpy as np
from scipy.interpolate import griddata
import plotly.graph_objects as go
from typing import Dict, Optional, Tuple

# --- Zone Delimitation ---
ZONES: Dict[str, Dict[str, float]] = {
    "cafeteria": {"xmin": 178.0, "xmax": 190.0, "ymin": 476.0, "ymax": 486.0},
    "library":   {"xmin": 170.0, "xmax": 180.0, "ymin": 460.0, "ymax": 475.0},
    "campus":    {"xmin": 1000.0, "xmax": 1000.0, "ymin": 0.0, "ymax": 0.0},
}

# --- Postgres ---
DB_HOST = os.getenv("PGHOST", "localhost")          # DB Location - Local
DB_PORT = os.getenv("PGPORT", "5432")               # Port
DB_NAME = os.getenv("PGDATABASE", "IoT1")           # DB name
DB_USER = os.getenv("PGUSER", "postgres")           # Db's owner
DB_PASS = os.getenv("PGPASSWORD", 'vivaHAASf1"')    # Password


# --- Conection, download and preProcessing ---
# Database connection handler
def get_conn():
    conn_str = f"host={DB_HOST} port={DB_PORT} dbname={DB_NAME} user={DB_USER}"
    if DB_PASS:
        conn_str += f" password={DB_PASS}"
    return psycopg.connect(conn_str)

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
    with get_conn() as conn:
        df = pd.read_sql(q, conn)

    if df.empty:
        return df

    # Loads into a pandas DataFrame (df)
    df["ts"] = pd.to_datetime(df["ts"])
    # Converts numeric fields to floats
    for col in ["rssi", "down", "up", "lat", "lon"]:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")

    df = df.dropna(subset=["lat", "lon"])       # Drops rows without coords
    return df

# --- Filter ---
def filter_by_zone(df: pd.DataFrame, zone: str) -> pd.DataFrame:
    # Checks existing zone
    if zone not in ZONES:
        raise ValueError(f"Unknown zone: {zone}")
    z = ZONES[zone]

    # Filters data by zone
    return df[
        (df["lat"].between(z["xmin"], z["xmax"])) &
        (df["lon"].between(z["ymin"], z["ymax"]))
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
    return (
        df.groupby("hour")[metric]
          .mean()
          .reset_index()
          .sort_values("hour")
    )

# --- Plot ---
# Returns an HTML string containing a 3D surface plot.
# FastAPI can directly return this as HTMLResponse.
def generate_3d_surface(df: pd.DataFrame, metric: str = "down") -> str:
    if df.empty:
        return "<h1>No data to show</h1>"

    if metric not in ["down", "up", "rssi"]:
        raise ValueError("metric must be 'down', 'up', or 'rssi'")

    x = df["lat"].values
    y = df["lon"].values
    z = df[metric].values

    xi = np.linspace(min(x), max(x), 100)
    yi = np.linspace(min(y), max(y), 100)
    Xi, Yi = np.meshgrid(xi, yi)

    Zi = griddata((x, y), z, (Xi, Yi), method="linear")

    fig = go.Figure(data=[go.Surface(x=Xi, y=Yi, z=Zi)])
    fig.update_layout(title=f"3D Surface: {metric}", autosize=True)

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
