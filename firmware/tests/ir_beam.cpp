// IR beam-break sensor test – ball-in-chamber detection
// Flash:   pio run -e test_irbeam -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Reads the IR beam-break sensor in both digital and analog modes, printing
// continuously.  The BOOT button is read in parallel as a wiring sanity check.
//
// Expected output:
//   IR: dig=1 adc=4095 (CLEAR)  |  BOOT: raw=1    ← beam unbroken
//   IR: dig=0 adc=  12 (BALL)   |  BOOT: raw=1    ← beam blocked by ball
//
// If the pin is always HIGH or always LOW regardless of blocking the beam,
// check wiring and that the receiver module has power.
//
// Wiring:
//   IR receiver output → GPIO15  (HIGH = clear, LOW = ball present)
//
// Course concepts: digital I/O, analog input (ADC), sensor interfacing

#include <Arduino.h>

static constexpr int PIN_IR_BEAM  = 15;
static constexpr int PIN_BOOT_BTN = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  IR Beam break sensor test v2");
    Serial.printf("  IR=GPIO%d  BOOT=GPIO%d (sanity)\n", PIN_IR_BEAM, PIN_BOOT_BTN);
    Serial.println("─────────────────────────────────────");
    Serial.println("Hold BOOT to verify code works.");
    Serial.println("Then block/unblock the IR beam.\n");

    pinMode(PIN_IR_BEAM,  INPUT);
    pinMode(PIN_BOOT_BTN, INPUT);
}

void loop() {
    int ir_raw   = digitalRead(PIN_IR_BEAM);
    int ir_adc   = analogRead(PIN_IR_BEAM);
    int boot_raw = digitalRead(PIN_BOOT_BTN);

    Serial.printf("IR: dig=%d adc=%d (%s)  |  BOOT: raw=%d\n",
                  ir_raw, ir_adc, ir_raw == LOW ? "BALL" : "CLEAR", boot_raw);

    delay(500);
}
