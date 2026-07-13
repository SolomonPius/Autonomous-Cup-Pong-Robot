// Pong Robot – main firmware (Scan → Aim → Fire)
//
// Flash:   pio run -e esp32dev -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Sequence (state machine driven from loop()):
//   SCAN  – Sweep servo 25°→120° and back, reading TF-Luna at every step.
//           Print angle and distance for each sample. Track the closest
//           valid reading as the target.
//   AIM   – Rotate servo to the best angle (or midpoint if no valid samples).
//   FIRE  – Flywheel kick-start → settle to FLY_DUTY → indexer ON → stop all.
//   DONE  – Terminal state; all motors off.
//
// Wiring:
//   Servo            → GPIO4   (50 Hz PWM via LEDC)
//   TF-Luna SDA/SCL  → GPIO21 / GPIO22  (I2C, 400 kHz fast-mode)
//   L298N (flywheels):
//     EN_A=GPIO13 IN1=GPIO14 IN2=GPIO16  (Motor A)
//     EN_B=GPIO27 IN3=GPIO17 IN4=GPIO23  (Motor B)
//   BTS7960 (indexer):
//     RPWM=GPIO18 LPWM=GPIO19 R_EN=GPIO25 L_EN=GPIO26
//
// Course concepts demonstrated:
//   - State machine       → RobotState enum + switch in loop()
//   - I2C protocol        → TF-Luna sensor reads via Wire library
//   - PWM                 → LEDC peripheral for motor speed + servo position
//   - Macros              → BYTES_TO_U16 (little-endian byte reconstruction)
//   - Inline functions    → setFlywheelDuty(), stopAllMotors()
//   - Code reuse          → lunaReadCm(), slewServoTo(), sampleAt() subroutines
//   - constexpr constants → compile-time pin/config definitions (zero RAM cost)
//   - Loops & branches    → sweep loops, aim decision logic

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>

// ── Macros ──────────────────────────────────────────────────────────────────
// Reconstruct a little-endian 16-bit value from two bytes (Week 2: Numbering Systems)
#define BYTES_TO_U16(lo, hi)  ((uint16_t)(lo) | ((uint16_t)(hi) << 8))

// ── Servo / LiDAR pins ───────────────────────────────────────────────────────
static constexpr int PIN_SERVO = 4;
static constexpr int PIN_SDA   = 21;
static constexpr int PIN_SCL   = 22;

// ── TF-Luna I2C ──────────────────────────────────────────────────────────────
static constexpr uint8_t TFLUNA_ADDR = 0x10;
static constexpr uint8_t REG_DIST_L  = 0x00;

// ── Sweep config ─────────────────────────────────────────────────────────────
static constexpr int SWEEP_MIN_DEG  = 25;
static constexpr int SWEEP_MAX_DEG  = 120;
static constexpr int SWEEP_STEP_DEG = 2;
static constexpr int SETTLE_MS      = 120;   // servo settling time per step
static constexpr int SCAN_CYCLES    = 1;     // number of out-and-back sweeps

// ── Slow-slew config (used when rotating to target after scan) ───────────────
static constexpr int SLEW_STEP_DEG = 1;       // degrees per step
static constexpr int SLEW_STEP_MS  = 25;      // delay between steps
// 1°/25 ms → full 95° range takes ~2.4 s, smooth motion

static int currentServoDeg = 0;               // tracks last commanded angle

// ── L298N pins (flywheels) ───────────────────────────────────────────────────
static constexpr int PIN_EN_A = 13;
static constexpr int PIN_IN1  = 14;
static constexpr int PIN_IN2  = 16;
static constexpr int PIN_IN3  = 17;
static constexpr int PIN_IN4  = 23;
static constexpr int PIN_EN_B = 27;

// ── BTS7960 pins (indexer) ───────────────────────────────────────────────────
static constexpr int PIN_IDX_RPWM = 18;
static constexpr int PIN_IDX_LPWM = 19;
static constexpr int PIN_IDX_R_EN = 25;
static constexpr int PIN_IDX_L_EN = 26;

// ── PWM config ───────────────────────────────────────────────────────────────
// NOTE: ESP32Servo grabs LEDC channels from the pool starting at 0, so we
// reserve high-numbered channels (different timer group) to avoid ever
// having ledcSetup() clobber the servo's PWM frequency.
static constexpr int LEDC_CH_ENA   = 8;   // L298N Motor A
static constexpr int LEDC_CH_ENB   = 9;   // L298N Motor B
static constexpr int LEDC_CH_IDX   = 10;  // BTS7960 indexer

static constexpr int L298N_FREQ_HZ = 1000;
static constexpr int BTS_FREQ_HZ   = 20000;
static constexpr int PWM_RES_BITS  = 8;

static constexpr uint8_t FLY_KICK_DUTY = 120;  // kick-start duty
static constexpr uint8_t FLY_DUTY      = 42;   // settled duty (~20%)
static constexpr uint8_t IDX_DUTY      = 150;
static constexpr int     FLY_KICK_MS   = 300;
static constexpr int     FLY_SETTLE_MS = 6000;
static constexpr int     IDX_SPIN_MS   = 50000;

static Servo servo;

// ── State machine (Week 12: State Machines) ─────────────────────────────────
enum class RobotState { SCAN, AIM, FIRE, DONE };
static RobotState state = RobotState::SCAN;

// Scan results persisted across states
static int bestAngle = -1;
static int bestDist  = INT_MAX;

// ── Inline helper functions (Week 7: Compilation & Optimization) ────────────
// inline avoids function-call overhead while keeping the code DRY.

static inline void setFlywheelDuty(uint8_t duty) {
    ledcWrite(LEDC_CH_ENA, duty);
    ledcWrite(LEDC_CH_ENB, duty);
}

static inline void stopAllMotors() {
    setFlywheelDuty(0);
    ledcWrite(LEDC_CH_IDX, 0);
}

// ── Subroutines (Week 6: Code Reuse and Subroutines) ────────────────────────

// Read distance from TF-Luna in cm. Returns -1 on error.
static int lunaReadCm() {
    Wire.beginTransmission(TFLUNA_ADDR);
    Wire.write(REG_DIST_L);
    if (Wire.endTransmission(false) != 0) return -1;
    if (Wire.requestFrom((uint8_t)TFLUNA_ADDR, (uint8_t)6) != 6) return -1;

    uint8_t buf[6];
    for (int i = 0; i < 6; i++) buf[i] = Wire.read();

    return BYTES_TO_U16(buf[0], buf[1]);
}

// Slew the servo from currentServoDeg to targetDeg in small steps so the
// turret doesn't jerk. Blocks until done. Updates currentServoDeg.
static void slewServoTo(int targetDeg) {
    int step = (targetDeg > currentServoDeg) ? SLEW_STEP_DEG : -SLEW_STEP_DEG;
    while (currentServoDeg != targetDeg) {
        int next = currentServoDeg + step;
        // Clamp on last step if we'd overshoot
        if ((step > 0 && next > targetDeg) || (step < 0 && next < targetDeg)) {
            next = targetDeg;
        }
        servo.write(next);
        currentServoDeg = next;
        delay(SLEW_STEP_MS);
    }
}

// Move servo to deg, settle, then sample LiDAR once and track best.
// Updates bestAngle / bestDist in place.
static void sampleAt(int deg, int &bestAngle, int &bestDist) {
    servo.write(deg);
    currentServoDeg = deg;
    delay(SETTLE_MS);

    int dist = lunaReadCm();
    if (dist > 0) {
        Serial.printf("angle=%3d°  dist=%4d cm\n", deg, dist);
        if (dist < bestDist) {
            bestDist  = dist;
            bestAngle = deg;
        }
    } else {
        Serial.printf("angle=%3d°  dist=  -- cm  (read error)\n", deg);
    }
}

// ── Hardware initialisation (runs once) ─────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  Pong Robot: scan → aim → fire");
    Serial.println("─────────────────────────────────────");

    // ── I2C + TF-Luna ────────────────────────────────────────────────────────
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    Wire.beginTransmission(TFLUNA_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("ERROR: TF-Luna not responding.");
        while (true) delay(1000);
    }
    Serial.println("TF-Luna OK.");

    // ── L298N flywheel setup (idle) ──────────────────────────────────────────
    // IMPORTANT: initialize motor PWM BEFORE servo.attach() so we cannot
    // accidentally reconfigure a LEDC channel the servo library has already
    // claimed. Combined with the high channel numbers (8/9/10), this keeps
    // the 50 Hz servo signal untouched.
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);

    ledcSetup(LEDC_CH_ENA, L298N_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_EN_A, LEDC_CH_ENA);
    ledcSetup(LEDC_CH_ENB, L298N_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_EN_B, LEDC_CH_ENB);
    setFlywheelDuty(0);

    // ── BTS7960 indexer setup (idle) ─────────────────────────────────────────
    pinMode(PIN_IDX_R_EN, OUTPUT);
    pinMode(PIN_IDX_L_EN, OUTPUT);
    digitalWrite(PIN_IDX_R_EN, HIGH);
    digitalWrite(PIN_IDX_L_EN, HIGH);
    ledcSetup(LEDC_CH_IDX, BTS_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_IDX_RPWM, LEDC_CH_IDX);
    pinMode(PIN_IDX_LPWM, OUTPUT);
    digitalWrite(PIN_IDX_LPWM, LOW);
    ledcWrite(LEDC_CH_IDX, 0);

    // ── Servo (after motors, so ESP32Servo grabs a free LEDC channel) ───────
    servo.setPeriodHertz(50);
    if (servo.attach(PIN_SERVO, 500, 2500) < 0) {
        Serial.println("ERROR: servo.attach() failed.");
        while (true) delay(1000);
    }
    // Assume worst case (servo at center) and slew gently to the start angle.
    currentServoDeg = 90;
    servo.write(currentServoDeg);
    delay(500);
    slewServoTo(SWEEP_MIN_DEG);
    delay(200);
}

// ── Main loop: state machine drives the scan → aim → fire sequence ──────────
void loop() {
    switch (state) {

    case RobotState::SCAN:
        Serial.printf("\n=== SCAN (%d cycle(s)) ===\n", SCAN_CYCLES);
        for (int cycle = 0; cycle < SCAN_CYCLES; cycle++) {
            for (int deg = SWEEP_MIN_DEG; deg <= SWEEP_MAX_DEG; deg += SWEEP_STEP_DEG) {
                sampleAt(deg, bestAngle, bestDist);
            }
            for (int deg = SWEEP_MAX_DEG; deg >= SWEEP_MIN_DEG; deg -= SWEEP_STEP_DEG) {
                sampleAt(deg, bestAngle, bestDist);
            }
        }
        state = RobotState::AIM;
        break;

    case RobotState::AIM: {
        Serial.println("\n=== SCAN COMPLETE ===");
        int aimDeg;
        if (bestAngle < 0) {
            aimDeg = (SWEEP_MIN_DEG + SWEEP_MAX_DEG) / 2;
            Serial.println("WARNING: no valid LiDAR readings during scan.");
            Serial.printf("Minimum angle: (none)  →  rotating to midpoint %d° (straight ahead)\n", aimDeg);
        } else {
            aimDeg = bestAngle;
            Serial.printf("Minimum distance: %d cm at angle %d°\n", bestDist, bestAngle);
            Serial.printf("Rotating to %d°...\n", aimDeg);
        }
        slewServoTo(aimDeg - 2);
        delay(300);
        Serial.println("Aim complete.");
        state = RobotState::FIRE;
        break;
    }

    case RobotState::FIRE:
        Serial.println("Flywheels kick-start at 120/255.");
        setFlywheelDuty(FLY_KICK_DUTY);
        ledcWrite(LEDC_CH_IDX, 0);
        delay(FLY_KICK_MS);

        Serial.printf("Flywheels settled to %u/255. Spinning up...\n", FLY_DUTY);
        setFlywheelDuty(FLY_DUTY);
        delay(FLY_SETTLE_MS);

        Serial.printf("Indexer ON at duty=%u\n", IDX_DUTY);
        ledcWrite(LEDC_CH_IDX, IDX_DUTY);
        delay(IDX_SPIN_MS);

        stopAllMotors();
        Serial.println("All motors stopped. Done.");
        state = RobotState::DONE;
        break;

    case RobotState::DONE:
        break;
    }
}
