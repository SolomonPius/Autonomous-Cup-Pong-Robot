#!/usr/bin/env python3
"""
radar_view.py - Live radar-style polar plot for ESP32 LiDAR sweep data.

Usage:
    python3 radar_view.py [SERIAL_PORT]

    SERIAL_PORT defaults to /dev/tty.usbserial-0001

    The ESP32 should send lines formatted as:
        angle,distance_cm
    e.g. "45,120" means 45 degrees at 120 cm.
    Any lines that don't match are silently ignored.

Requires: pyserial, matplotlib
    pip install pyserial matplotlib
"""

import sys
import math
import time
import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
DEFAULT_PORT = "/dev/tty.usbserial-0001"
BAUD_RATE = 115200
ANGLE_MIN = 0      # degrees
ANGLE_MAX = 120    # degrees
DIST_MAX = 150      # cm – outer ring of the radar
FADE_TIME_S = 2.0  # seconds before a point fully fades out
UPDATE_INTERVAL_MS = 50  # milliseconds between animation frames

# ---------------------------------------------------------------------------
# Serial helper
# ---------------------------------------------------------------------------

def open_serial(port: str) -> serial.Serial:
    """Open the serial port with a short timeout so reads are non-blocking."""
    return serial.Serial(port, BAUD_RATE, timeout=0.02)


def parse_line(line: str):
    """Return (angle_deg, distance_cm) or None if the line is not valid."""
    line = line.strip()
    parts = line.split(",")
    if len(parts) != 2:
        return None
    try:
        angle = float(parts[0])
        dist = float(parts[1])
    except ValueError:
        return None
    if not (ANGLE_MIN <= angle <= ANGLE_MAX):
        return None
    if dist < 0:
        return None
    return angle, dist

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    print(f"Opening {port} at {BAUD_RATE} baud …")
    ser = open_serial(port)
    print("Connected. Waiting for data …")

    # -- Set up the polar plot ------------------------------------------------
    fig = plt.figure(figsize=(8, 8), facecolor="black")
    ax = fig.add_subplot(111, polar=True, facecolor="black")

    # Angle axis: show 0-120 degrees, with 0 at the top
    ax.set_thetamin(ANGLE_MIN)
    ax.set_thetamax(ANGLE_MAX)
    ax.set_theta_zero_location("N")   # 0 degrees points up
    ax.set_theta_direction(-1)         # clockwise

    # Distance axis
    ax.set_ylim(0, DIST_MAX)
    ring_ticks = np.arange(10, DIST_MAX + 1, 10)
    ax.set_yticks(ring_ticks)
    ax.set_yticklabels([f"{int(r)} cm" for r in ring_ticks],
                       fontsize=7, color="green")
    ax.tick_params(axis="x", colors="green", labelsize=8)
    ax.grid(color="green", alpha=0.3, linewidth=0.5)
    ax.set_title("LiDAR Radar View", color="lime", fontsize=14, pad=20)

    # Points buffer: each entry is (theta, r, timestamp)
    points: list[tuple[float, float, float]] = []

    # Use a scatter plot so we can set per-point alpha via RGBA colors
    scatter = ax.scatter([], [], s=16, c=[], marker="o")

    # Sweep line
    sweep_line, = ax.plot([], [], "-", color="lime", linewidth=1.5, alpha=0.7)

    # -- Animation callback ---------------------------------------------------
    def update(_frame):
        nonlocal points

        now = time.monotonic()

        # Read all available serial data
        lines_read = 0
        while lines_read < 200:  # cap per frame to keep UI responsive
            try:
                raw = ser.readline()
                if not raw:
                    break
                line = raw.decode("utf-8", errors="ignore")
            except Exception:
                break

            parsed = parse_line(line)
            if parsed is None:
                lines_read += 1
                continue

            angle, dist = parsed
            theta = math.radians(angle)
            points.append((theta, min(dist, DIST_MAX), now))

            print(f"  {angle:5.1f}°  {dist:6.1f} cm")
            lines_read += 1

        # Prune expired points
        points = [(t, r, ts) for t, r, ts in points if now - ts < FADE_TIME_S]

        if points:
            thetas = [p[0] for p in points]
            rs = [p[1] for p in points]
            ages = [now - p[2] for p in points]

            # RGBA: lime with alpha fading from 1.0 → 0.0 over FADE_TIME_S
            colors = [(0.2, 1.0, 0.2, max(0.0, 1.0 - age / FADE_TIME_S))
                      for age in ages]

            scatter.set_offsets(np.column_stack((thetas, rs)))
            scatter.set_color(colors)

            # Sweep line to latest point
            sweep_line.set_data(
                [thetas[-1], thetas[-1]], [0, DIST_MAX]
            )
        else:
            scatter.set_offsets(np.empty((0, 2)))
            sweep_line.set_data([], [])

        return (scatter, sweep_line)

    ani = animation.FuncAnimation(           # noqa: F841
        fig, update, interval=UPDATE_INTERVAL_MS, blit=True, cache_frame_data=False
    )

    plt.tight_layout()
    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
        print("Serial port closed.")


if __name__ == "__main__":
    main()
