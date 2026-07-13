// L298N motor-driver test – Motor A speed ramp cycle
// Flash:   pio run -e test_l298n -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Continuously ramps Motor A duty from DUTY_MIN → DUTY_MAX → DUTY_MIN using
// the ESP32 LEDC peripheral for hardware PWM generation.  Motor B is held
// disabled to isolate the test.
//
// Wiring:
//   EN_A → GPIO13   (PWM speed control via LEDC)
//   IN1  → GPIO14   (direction)
//   IN2  → GPIO16   (direction)
//   IN3  → GPIO17   (Motor B direction – held LOW)
//   IN4  → GPIO23   (Motor B direction – held LOW)
//   EN_B → GPIO27   (Motor B enable  – held LOW)
//
// Course concepts: PWM via LEDC timer peripheral, H-bridge direction control,
//                  loop-based ramp pattern

#include <Arduino.h>

// ── Pin assignments ──────────────────────────────────────────────────────────
static constexpr int PIN_EN_A = 13;
static constexpr int PIN_IN1  = 14;
static constexpr int PIN_IN2  = 16;
static constexpr int PIN_IN3  = 17;
static constexpr int PIN_IN4  = 23;
static constexpr int PIN_EN_B = 27;

// ── PWM config ───────────────────────────────────────────────────────────────
static constexpr int LEDC_CH_ENA       = 2;
static constexpr int LEDC_PWM_FREQ_HZ  = 1000;
static constexpr int LEDC_PWM_RES_BITS = 8;     // 0–255 duty range

// ── Ramp config ──────────────────────────────────────────────────────────────
static constexpr uint8_t DUTY_MIN    = 80;   // low end of ramp
static constexpr uint8_t DUTY_MAX    = 255;  // full speed
static constexpr int     STEP_SIZE   = 5;    // duty change per step
static constexpr int     STEP_MS     = 50;   // ms between steps

static uint8_t duty    = DUTY_MIN;
static int8_t  direction = 1;  // +1 = ramping up, -1 = ramping down

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  L298N Motor A test – speed cycle");
    Serial.printf("  EN_A=GPIO%d  IN1=GPIO%d  IN2=GPIO%d\n",
                  PIN_EN_A, PIN_IN1, PIN_IN2);
    Serial.println("─────────────────────────────────────\n");

    // Direction pins – Motor A forward
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);

    // Motor B pins – keep disabled so only Motor A runs
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);
    pinMode(PIN_EN_B, OUTPUT);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, LOW);
    digitalWrite(PIN_EN_B, LOW);

    // PWM on EN_A for speed control
    ledcSetup(LEDC_CH_ENA, LEDC_PWM_FREQ_HZ, LEDC_PWM_RES_BITS);
    ledcAttachPin(PIN_EN_A, LEDC_CH_ENA);
    ledcWrite(LEDC_CH_ENA, duty);
}

void loop() {
    int next = duty + direction * STEP_SIZE;

    if (next >= DUTY_MAX) {
        duty = DUTY_MAX;
        direction = -1;
    } else if (next <= DUTY_MIN) {
        duty = DUTY_MIN;
        direction = 1;
    } else {
        duty = next;
    }

    ledcWrite(LEDC_CH_ENA, duty);
    Serial.printf("duty=%3d/255 (~%2d%%)  %s\n",
                  duty, duty * 100 / 255,
                  direction > 0 ? "ramping up" : "ramping down");
    delay(STEP_MS);
}
