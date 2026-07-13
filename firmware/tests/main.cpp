// MG995 servo test – turret rotation validation
// Flash:   pio run -e test_servo -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Continuously sweeps the servo from 0°→120°→0° using non-blocking millis()
// timing.  Also accepts angle commands via Serial Monitor: type an integer
// (0–120) and press enter to jump to that position; the sweep resumes from
// the new position.
//
// Wiring:
//   Servo signal → GPIO4  (50 Hz PWM, 500–2500 µs pulse range)
//
// Course concepts: hardware PWM (LEDC timer), non-blocking timing with
//                  millis(), UART serial input parsing

#include <Arduino.h>
#include <ESP32Servo.h>

#include "config.h"

static Servo servo;

static constexpr int SERVO_MIN_DEG = 0;
static constexpr int SERVO_MAX_DEG = 120;

static constexpr uint32_t STEP_PERIOD_MS = 100; // slow sweep
static constexpr int STEP_DEG = 1;

static int currentDeg = 90;
static int sweepDir = +1;

static void applyPosition(int deg) {
    deg = constrain(deg, SERVO_MIN_DEG, SERVO_MAX_DEG);
    servo.write(deg);
}

static void maybeReadAngleFromSerial() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    int val = line.toInt();
    if (val < SERVO_MIN_DEG || val > SERVO_MAX_DEG) {
        Serial.printf("Angle out of range: %d (use %d-%d)\n", val, SERVO_MIN_DEG, SERVO_MAX_DEG);
        return;
    }

    currentDeg = val;
    applyPosition(currentDeg);
    Serial.printf("Jumped to %d° (continuing sweep)\n", currentDeg);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  Servo test (MG995)");
    Serial.printf("  PIN_SERVO=GPIO%d\n", PIN_SERVO);
    Serial.println("  Sweep mode + serial angle override");
    Serial.printf("  Type an angle %d-%d and press enter\n", SERVO_MIN_DEG, SERVO_MAX_DEG);
    Serial.println("─────────────────────────────────────\n");

    servo.setPeriodHertz(50);
    const int ch = servo.attach(PIN_SERVO, 500, 2500);
    if (ch < 0) {
        Serial.println("ERROR: servo.attach() failed (no LEDC channel available?)");
        while (true) delay(1000);
    }
    Serial.printf("Servo attached on LEDC channel %d\n", ch);

    currentDeg = 0;
    applyPosition(currentDeg);
}

void loop() {
    maybeReadAngleFromSerial();

    // Non-blocking sweep using millis() instead of delay()
    static uint32_t lastStepMs = 0;
    const uint32_t now = millis();
    if (now - lastStepMs >= STEP_PERIOD_MS) {
        lastStepMs = now;

        currentDeg += sweepDir * STEP_DEG;
        if (currentDeg >= SERVO_MAX_DEG) {
            currentDeg = SERVO_MAX_DEG;
            sweepDir = -1;
        } else if (currentDeg <= SERVO_MIN_DEG) {
            currentDeg = SERVO_MIN_DEG;
            sweepDir = +1;
        }
        applyPosition(currentDeg);
    }

    static uint32_t lastPrintMs = 0;
    if (now - lastPrintMs >= 500) {
        lastPrintMs = now;
        Serial.printf("pos=%d°\n", currentDeg);
    }
}
