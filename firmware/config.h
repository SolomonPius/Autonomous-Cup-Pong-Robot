#pragma once

// ─── I2C ─────────────────────────────────────────────────────────────────────
#define PIN_SDA             21   // TF-Luna SDA
#define PIN_SCL             22   // TF-Luna SCL
#define TFLUNA_I2C_ADDR     0x10

// ─── BTS7960 H-Bridge (Flywheel motors – combined channel) ───────────────────
// NOTE: Both flywheel motors share one BTS7960 channel (known constraint).
#define PIN_FLY_RPWM        18
#define PIN_FLY_LPWM        19
#define PIN_FLY_R_EN        25
#define PIN_FLY_L_EN        26

// LEDC channels for BTS7960 PWM
#define LEDC_CH_RPWM        0
#define LEDC_CH_LPWM        1
#define LEDC_PWM_FREQ_HZ    20000  // 20 kHz – above audible range
#define LEDC_PWM_RES_BITS   8      // 0–255 duty cycle

// ─── MG995 Servo (Turret rotation) ───────────────────────────────────────────
#define PIN_SERVO           4

// ─── JGA25-371 Gearmotor + Quadrature Encoder (planned) ──────────────────────
// Motor driver (generic interface): PWM + DIR
// Encoder: A/B channels
//
// Chosen to avoid pins currently used by tests and common ESP32 bootstraps.
// - GPIO32/33 are input-only (good for encoder).
// - GPIO16/17 are general-purpose outputs (good for driver PWM/DIR).
#define PIN_GEAR_PWM        16
#define PIN_GEAR_DIR        17
#define PIN_GEAR_ENC_A      32
#define PIN_GEAR_ENC_B      33

// ─── Hall Effect Sensors (Flywheel RPM feedback, input-only pins) ─────────────
#define PIN_HALL_1          13   // Flywheel motor 1
#define PIN_HALL_2          14   // Flywheel motor 2

// ─── Limit Switches (Turret end-stops for homing) ────────────────────────────
#define PIN_LIMIT_CW        4   // Clockwise travel limit
#define PIN_LIMIT_CCW       5   // Counter-clockwise travel limit

// ─── IR Beam (Ball-in-chamber sensor, input-only) ────────────────────────────
#define PIN_IR_BEAM         27   // HIGH = beam unbroken (no ball); LOW = ball present

// ─── Feeder motor (DRV8833, 2nd-order part – pins TBD when hardware arrives) ──
#define PIN_FEED_IN1        -1   // Placeholder
#define PIN_FEED_IN2        -1   // Placeholder

// ─── User button ─────────────────────────────────────────────────────────────
#define PIN_BUTTON          0    // GPIO0 / BOOT button on DevKit (active-low)

// ─── Turret geometry ─────────────────────────────────────────────────────────
#define TURRET_HOME_DEG     90.0f  // Straight-ahead position
#define TURRET_MIN_DEG       0.0f
#define TURRET_MAX_DEG     180.0f

// ─── Scan parameters ─────────────────────────────────────────────────────────
#define SCAN_START_DEG      30.0f  // Left limit of scan arc
#define SCAN_END_DEG       150.0f  // Right limit of scan arc
#define SCAN_STEP_DEG        2.0f  // Angular resolution
#define SCAN_DWELL_MS          80  // ms to wait at each step before reading

// Cup detection: a return shorter than (background − DELTA) is a cup candidate
#define CUP_DIST_MIN_MM      300   // Minimum plausible cup distance
#define CUP_DIST_MAX_MM     2400   // Table length; farther is background
#define CUP_DETECT_DELTA_MM   80   // How much shorter than background = cup

// ─── Ballistics ──────────────────────────────────────────────────────────────
#define LAUNCH_ANGLE_DEG    55.0f   // Fixed vertical launch angle
#define LAUNCHER_HEIGHT_M   0.20f   // Launcher exit height above table surface
#define CUP_RIM_HEIGHT_M    0.12f   // Cup rim height above table surface
// Height difference seen by projectile: launcher is above cup rim
#define DELTA_HEIGHT_M      (LAUNCHER_HEIGHT_M - CUP_RIM_HEIGHT_M)

// Flywheel: ball exit velocity ≈ flywheel rim speed (nerf-style compression launch)
#define FLYWHEEL_RADIUS_M   0.030f  // Wheel radius in metres
// RPM → ball speed:  v = ω·r = (RPM·2π/60)·r
// Rearranged:        RPM = v·60 / (2π·r)

// Closed-loop RPM: the hall sensor produces one pulse per magnet pole per rev.
// Adjust HALL_POLES to match your motor's magnet count.
#define HALL_POLES           2      // Pulses per revolution
#define RPM_MEASURE_WINDOW_MS 200   // Integration window for RPM measurement
#define RPM_TOLERANCE        150    // ±RPM considered "at setpoint"

// ─── Timing ──────────────────────────────────────────────────────────────────
#define SPINUP_TIMEOUT_MS   5000   // Abort spinup if RPM not reached in this time
#define FIRE_PULSE_MS        600   // Duration feeder motor runs to push one ball
#define COOLDOWN_MS         1500   // Wait after shot before next cycle
