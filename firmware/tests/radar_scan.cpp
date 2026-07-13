// Radar scan test – servo sweep + TF-Luna LiDAR
// Flash:   pio run -e test_radar_scan -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Continuously sweeps the servo from SWEEP_MIN_DEG → SWEEP_MAX_DEG and back,
// reading the TF-Luna distance at each angular step.  Outputs CSV format
// (angle,distance_cm) for live plotting with tools/radar_view.py.
//
// Wiring:
//   Servo signal   → GPIO4   (50 Hz PWM, 500–2500 µs pulse range)
//   TF-Luna SDA    → GPIO21  (I2C data)
//   TF-Luna SCL    → GPIO22  (I2C clock, 400 kHz fast-mode)
//
// Course concepts: I2C protocol (sensor polling), servo PWM control,
//                  sweep/scan loop pattern, CSV serial output for host-side
//                  data visualisation

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>

// ── Pin assignments ──────────────────────────────────────────────────────────
static constexpr int PIN_SERVO = 4;
static constexpr int PIN_SDA   = 21;
static constexpr int PIN_SCL   = 22;

// ── TF-Luna I2C ─────────────────────────────────────────────────────────────
static constexpr uint8_t TFLUNA_ADDR = 0x10;
static constexpr uint8_t REG_DIST_L  = 0x00;

// ── Sweep config ─────────────────────────────────────────────────────────────
static constexpr int SWEEP_MIN_DEG    = 25;
static constexpr int SWEEP_MAX_DEG    = 120;
static constexpr int SWEEP_STEP_DEG   = 2;
static constexpr int SETTLE_MS        = 120;   // wait for servo to reach position

static Servo servo;
static int currentDeg = SWEEP_MIN_DEG;
static int sweepDir   = +1;

// Read distance from TF-Luna via I2C. Returns -1 on error.
static int lunaReadCm() {
    Wire.beginTransmission(TFLUNA_ADDR);
    Wire.write(REG_DIST_L);
    if (Wire.endTransmission(false) != 0) return -1;
    if (Wire.requestFrom((uint8_t)TFLUNA_ADDR, (uint8_t)6) != 6) return -1;

    uint8_t buf[6];
    for (int i = 0; i < 6; i++) buf[i] = Wire.read();

    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  Radar scan – servo + TF-Luna");
    Serial.printf("  Servo=GPIO%d  SDA=GPIO%d  SCL=GPIO%d\n", PIN_SERVO, PIN_SDA, PIN_SCL);
    Serial.println("─────────────────────────────────────");

    // I2C for TF-Luna at 400 kHz fast-mode
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    Wire.beginTransmission(TFLUNA_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("ERROR: TF-Luna not responding.");
        while (true) delay(1000);
    }
    Serial.println("TF-Luna OK.");

    servo.setPeriodHertz(50);
    int ch = servo.attach(PIN_SERVO, 500, 2500);
    if (ch < 0) {
        Serial.println("ERROR: servo.attach() failed.");
        while (true) delay(1000);
    }

    currentDeg = SWEEP_MIN_DEG;
    servo.write(currentDeg);
    delay(500);  // let servo reach start position

    Serial.println("Scanning...\n");
}

void loop() {
    servo.write(currentDeg);
    delay(SETTLE_MS);

    int dist = lunaReadCm();

    // CSV output: angle,distance_cm  (consumed by tools/radar_view.py)
    if (dist >= 0) {
        Serial.printf("%d,%d\n", currentDeg, dist);
    } else {
        Serial.printf("%d,-1\n", currentDeg);
    }

    // Advance sweep, reversing at limits
    currentDeg += sweepDir * SWEEP_STEP_DEG;
    if (currentDeg >= SWEEP_MAX_DEG) {
        currentDeg = SWEEP_MAX_DEG;
        sweepDir = -1;
    } else if (currentDeg <= SWEEP_MIN_DEG) {
        currentDeg = SWEEP_MIN_DEG;
        sweepDir = +1;
    }
}
