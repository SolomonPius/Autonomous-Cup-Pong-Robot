// Hall-effect sensor test – flywheel RPM feedback validation
// Flash:   pio run -e test_hall -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Reads two hall-effect sensors (one per flywheel motor) in both digital and
// analog modes.  Hold a magnet near each sensor and watch the values change.
// The BOOT button is read as a wiring sanity check.
//
// Wiring:
//   Hall sensor 1 → GPIO13  (flywheel motor 1)
//   Hall sensor 2 → GPIO14  (flywheel motor 2)
//
// Course concepts: digital I/O, analog input (ADC), polling loop

#include <Arduino.h>

static constexpr int PIN_HALL_1    = 13;
static constexpr int PIN_HALL_2    = 14;
static constexpr int PIN_BOOT_BTN  = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  Hall sensor test");
    Serial.printf("  H1=GPIO%d  H2=GPIO%d  BOOT=GPIO%d\n",
                  PIN_HALL_1, PIN_HALL_2, PIN_BOOT_BTN);
    Serial.println("─────────────────────────────────────");
    Serial.println("Hold a magnet near each sensor.\n");

    pinMode(PIN_HALL_1,   INPUT_PULLUP);
    pinMode(PIN_HALL_2,   INPUT_PULLUP);
    pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
}

void loop() {
    int h1   = digitalRead(PIN_HALL_1);
    int h2   = digitalRead(PIN_HALL_2);
    int boot = digitalRead(PIN_BOOT_BTN);
    int a1   = analogRead(PIN_HALL_1);
    int a2   = analogRead(PIN_HALL_2);

    Serial.printf("H1: dig=%d adc=%4d  |  H2: dig=%d adc=%4d  |  BOOT: %d\n",
                  h1, a1, h2, a2, boot);

    delay(500);
}
