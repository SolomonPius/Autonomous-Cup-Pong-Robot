// Flywheel motor test – BTS7960 H-bridge single-channel spin
// Flash:   pio run -e test_flywheel -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Enables the BTS7960 and drives the RPWM channel at a constant low duty
// cycle (~39% of 255) continuously.  LPWM is held LOW (forward direction
// only).  Status is printed once per second using non-blocking millis()
// timing so the motor output is never interrupted.
//
// Wiring:
//   BTS7960 RPWM → GPIO18  (PWM speed, 20 kHz via LEDC)
//   BTS7960 LPWM → GPIO19  (held LOW – forward only)
//   BTS7960 R_EN → GPIO25  (enable, driven HIGH)
//   BTS7960 L_EN → GPIO26  (enable, driven HIGH)
//
// Course concepts: high-frequency PWM via LEDC timer peripheral,
//                  H-bridge motor control, non-blocking timing

#include <Arduino.h>

static constexpr int PIN_FLY_RPWM = 18;
static constexpr int PIN_FLY_LPWM = 19;
static constexpr int PIN_FLY_R_EN   = 25;
static constexpr int PIN_FLY_L_EN   = 26;

static constexpr int LEDC_CH_RPWM     = 0;
static constexpr int LEDC_CH_LPWM     = 1;
static constexpr int LEDC_PWM_FREQ_HZ = 20000;   // 20 kHz – above audible range
static constexpr int LEDC_PWM_RES_BITS = 8;

static constexpr uint8_t SLOW_DUTY = 100;  // ~39% of 255

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  Flywheel motor test (BTS7960)");
    Serial.printf("  RPWM=GPIO%d  LPWM=GPIO%d  R_EN=GPIO%d  L_EN=GPIO%d\n",
                  PIN_FLY_RPWM, PIN_FLY_LPWM, PIN_FLY_R_EN, PIN_FLY_L_EN);
    Serial.println("─────────────────────────────────────\n");

    // Enable pins – both must be HIGH for the BTS7960 to drive
    pinMode(PIN_FLY_R_EN, OUTPUT);
    pinMode(PIN_FLY_L_EN, OUTPUT);
    digitalWrite(PIN_FLY_R_EN, LOW);
    digitalWrite(PIN_FLY_L_EN, LOW);

    // LEDC PWM setup for both channels
    ledcSetup(LEDC_CH_RPWM, LEDC_PWM_FREQ_HZ, LEDC_PWM_RES_BITS);
    ledcSetup(LEDC_CH_LPWM, LEDC_PWM_FREQ_HZ, LEDC_PWM_RES_BITS);
    ledcAttachPin(PIN_FLY_RPWM, LEDC_CH_RPWM);
    ledcAttachPin(PIN_FLY_LPWM, LEDC_CH_LPWM);
    ledcWrite(LEDC_CH_RPWM, 0);
    ledcWrite(LEDC_CH_LPWM, 0);

    // Start spinning: RPWM drives forward, LPWM held at 0
    digitalWrite(PIN_FLY_R_EN, HIGH);
    digitalWrite(PIN_FLY_L_EN, HIGH);
    ledcWrite(LEDC_CH_RPWM, SLOW_DUTY);
    ledcWrite(LEDC_CH_LPWM, 0);
}

void loop() {
    static uint32_t lastPrintMs = 0;
    const uint32_t now = millis();
    if (now - lastPrintMs >= 1000) {
        lastPrintMs = now;
        Serial.printf("Spinning continuously at duty=%d/255 (~%d%%)\n",
                      SLOW_DUTY,
                      SLOW_DUTY * 100 / 255);
    }
}
