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
"""ZONES: Dict[str, Dict[str, float]] = {
    "Cafe":     {"xmin": 260.0, "xmax": 310.0, "ymin": 336.0, "ymax": 400.0},
    "Biblio":   {"xmin": 165.0, "xmax": 218.0, "ymin": 392.0, "ymax": 425.0},
    "A1A2":     {"xmin": 170.0, "xmax": 267.0, "ymin": 404.0, "ymax": 539.0},
    "Life":     {"xmin": 0.0, "xmax": 200.0, "ymin": 540.0, "ymax": 700.0},
    "Aulas4":   {"xmin": 367.0, "xmax": 591.0, "ymin": 50.0, "ymax": 400.0},
}"""

ZONES: Dict[str, Dict[str, float]] = {
    "Cafe":     {"xmin": 336.0, "xmax": 400.0, "ymin": 260.0, "ymax": 310.0},
    "Biblio":   {"xmin": 392.0, "xmax": 425.0, "ymin": 165.0, "ymax": 218.0},
    "A1A2":     {"xmin": 404.0, "xmax": 539.0, "ymin": 170.0, "ymax": 267.0},
    "Life":     {"xmin": 540.0, "xmax": 700.0, "ymin": 0.0, "ymax": 200.0},
    "Aulas4":   {"xmin": 50.0, "xmax": 400.0, "ymin": 367.0, "ymax": 591.0},
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
    ORDER BY ts
    """
    # WHERE ts >= now() - interval '{hours} hours'

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

    df = df.dropna(subset=["lat", "lon", "down", "up"])       # Drops rows without coords or data
    return df

# --- Filter ---
def filter_by_zone(df: pd.DataFrame, zone: str) -> pd.DataFrame:
    # Checks existing zone
    if zone not in ZONES:
        raise ValueError(f"Unknown zone: {zone}")
    z = ZONES[zone]

    # Filters data by zone
    return df[
        (df["lon"].between(z["xmin"], z["xmax"])) &
        (df["lat"].between(z["ymin"], z["ymax"]))
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
def generate_3d_surface(df: pd.DataFrame, metric: str = "down", hour_start: int | None = None, hour_end: int | None = None) -> str:
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
