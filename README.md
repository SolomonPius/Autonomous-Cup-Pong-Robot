# Autonomous Cup Pong Robot

An autonomous turret robot that detects, aims at, and launches ping-pong balls into cups — built for MIE438 (Embedded Systems) at the University of Toronto.

**[Demo Video](https://www.youtube.com/watch?v=Q0vxaehKVus)**

Built by Solomon Pius, Abanoub Bashara, Roy Bou Abboud, and Yiyi Xu.

---

## Overview

The robot follows a sense-plan-act architecture: a rotating 1D LiDAR (TF-Luna) scans the playing field to find the closest cup, the ESP32 computes the required turret angle, and a servo-actuated turret aims and fires a ping-pong ball using dual flywheel motors and a feeder mechanism.

The project began as a full computer-vision-based aiming system (see [Project Proposal](docs/process/MIE438_Project_Proposal.pdf)) but was scoped down mid-project to a single-board, LiDAR-based design to fit the course timeline while preserving the core sense-plan-act architecture — full reasoning in the [Final Report](docs/process/MIE438_Final_Report.pdf).

**Final performance:**
- 100% cup detection rate
- ±2° angular accuracy
- Reliable shooting range up to 200cm

---

## Features

- **Perception:** TF-Luna LiDAR mounted co-axially with the turret barrel, scanning 5°–120° in 2° increments over I²C (400kHz) to find the minimum distance to a cup.
- **Aiming:** MG995 servo turret, slewed at a controlled max rate (2°/25ms) to avoid structural torque/instability.
- **Launch:** Dual brushless flywheel motors (L298N driver, 1kHz PWM) ramped to a fixed RPM, paired with a feeder motor (BTS7960 driver, 20kHz PWM) to advance the ball into the launcher.
- **Control:** Single ESP32-Wroom-32E running Arduino Framework C++ (PlatformIO), using a state machine (Scan → Aim → Fire → Done) to sequence the shooting cycle.
- **Custom hardware:** Fully custom 3D-printed mechanical structure, including a self-designed lazy susan bearing for turret rotation.

---

## System Architecture

```
[TF-Luna LiDAR] --I2C--> [ESP32] --PWM--> [Servo Turret]
                             |
                             +--PWM--> [Flywheel Motors (L298N)]
                             |
                             +--PWM--> [Feeder Motor (BTS7960)]
```

Full pinout and subsystem table in [docs/electrical/](docs/electrical/).

---

## Repo Structure

```
├── firmware/
│   ├── src/main.cpp          # Main control loop & state machine
│   └── tests/                # Per-peripheral bring-up test programs
│       ├── tfluna/
│       ├── servo/
│       ├── radar_scan/
│       ├── l298n/
│       ├── flywheel/
│       ├── hall_sensor/
│       ├── ir_beam/
│       └── all_motors/
│   └── tools/
│       └── radar_view.py     # Python visualizer for LiDAR polar scan
├── docs/
│   ├── electrical/            # Schematic, BOM, pinout diagram
│   ├── mechanical/             # CAD assembly, final assembly photos
│   ├── software/                # Architecture diagram, firmware flowchart
│   └── process/                  # Proposal, final report, performance metrics, instructor feedback
└── README.md
```

---

## Hardware

| Subsystem | Device | ESP32 Pin(s) | Protocol |
|---|---|---|---|
| LiDAR | TF-Luna (0x10) | SDA=GPIO21, SCL=GPIO22 | I²C @ 400kHz |
| Turret | MG995 servo | GPIO4 | 50Hz PWM (ESP32Servo) |
| Flywheel A | L298N EN_A/IN1/IN2 | 13/14/16 | 1kHz PWM (LEDC ch 8) |
| Flywheel B | L298N EN_B/IN3/IN4 | 27/17/23 | 1kHz PWM (LEDC ch 9) |
| Indexer | BTS7960 RPWM/LPWM/R_EN/L_EN | 18/19/25/26 | 20kHz PWM (LEDC ch 10) |

Power: 12V LiPo for motors, LM2596 buck converter to 5V for servo/LiDAR, 3.3V from ESP32 for remaining components.

**Note:** The ball feeder tube CAD was sourced from [Tylr-J42/XRP-Ping-Pong-Ball-Shooter](https://github.com/Tylr-J42/XRP-Ping-Pong-Ball-Shooter) and is not original work — all other mechanical parts were custom-designed in Onshape.

### CAD Files
 
- **Live/editable:** [Onshape Document](https://cad.onshape.com/documents/006694550e54c6535ae3a296/w/fde7019724d253a4cde574ab/e/71389e919a5da7285be2d17a) — full feature tree, editable, always reflects the latest version.
- **Static/portable:** [Final_CAD_Assembly](docs/mechanical/Final_CAD_Assembly.step) — exported STEP file, viewable in any CAD tool, kept as a backup in case the Onshape link ever changes or becomes inaccessible.
---


## Testing & Validation

Each peripheral was bring-up tested independently before full integration (see `firmware/tests/`):
- `tfluna` — I²C bring-up and distance accuracy
- `servo` — end-to-end MG995 sweep
- `radar_scan` — combined servo + LiDAR polar scan (visualized via `radar_view.py`)
- `l298n` / `flywheel` — flywheel driver activation
- `all_motors` — full firing sequence, end-to-end

---

## Known Limitations & Future Work

- Flywheel motors run at a fixed shared duty cycle — a closed-loop RPM control system (hall-effect sensor + PID) was designed but not completed due to hardware issues.
- The custom-printed turret bearing introduced some tilt/instability during rotation.
- An IR beam sensor for shot-state detection was attempted but never worked reliably in testing.
- Planned upgrades: 2D LiDAR (replacing the rotating scan), full autonomy-stack state machine, working IR beam integration.

Full reflection in the [Final Report](docs/process/MIE438_Final_Report.pdf).

---

## AI Usage Disclosure

In accordance with course guidelines: Claude and Claude Code were used for generating/refining inline code comments and documentation, and for debugging assistance (peripheral bring-up issues, sensor behavior interpretation). All architectural decisions, hardware design, firmware logic, and testing were performed by the team.
