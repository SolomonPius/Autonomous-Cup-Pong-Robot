// Limit-switch test – verifies turret end-stop wiring
// Flash:   pio run -e test_limits -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Reads the two limit switches (CW and CCW turret travel) plus the onboard
// BOOT button as a sanity reference.  Prints raw digital state continuously.
//
// Expected output (prints on change only):
//   CW:  OPEN   CCW: OPEN
//   CW:  HIT    CCW: OPEN    ← press CW switch
//   CW:  OPEN   CCW: HIT     ← press CCW switch
//
// If a switch always reads HIT, check the external pull-up resistor.
// If a switch never reads HIT when pressed, check wiring.
//
// Wiring:
//   CW  limit switch → GPIO4   (active-low, internal pull-up enabled)
//   CCW limit switch → GPIO5   (active-low, internal pull-up enabled)
//
// Course concepts: digital I/O, internal pull-up resistors, polling loop

#include <Arduino.h>

static constexpr int PIN_LIMIT_CW  = 4;
static constexpr int PIN_LIMIT_CCW = 5;
static constexpr int PIN_BOOT_BTN  = 0;   // onboard BOOT button — sanity check

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  Limit switch test");
    Serial.printf("  CW=GPIO%d  CCW=GPIO%d  BOOT=GPIO%d\n",
                  PIN_LIMIT_CW, PIN_LIMIT_CCW, PIN_BOOT_BTN);
    Serial.println("─────────────────────────────────────");
    Serial.println("Hold the BOOT button to verify code works.");
    Serial.println("Then test limit switches.\n");

    pinMode(PIN_LIMIT_CW,  INPUT_PULLUP);
    pinMode(PIN_LIMIT_CCW, INPUT_PULLUP);
    pinMode(PIN_BOOT_BTN,  INPUT_PULLUP);
}

void loop() {
    int cw_raw   = digitalRead(PIN_LIMIT_CW);
    int ccw_raw  = digitalRead(PIN_LIMIT_CCW);
    int boot_raw = digitalRead(PIN_BOOT_BTN);

    Serial.printf("CW: raw=%d  |  CCW: raw=%d  |  BOOT: raw=%d\n",
                  cw_raw, ccw_raw, boot_raw);

    delay(500);
}
