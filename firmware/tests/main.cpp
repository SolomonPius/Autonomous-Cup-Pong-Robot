// All-motors sequenced test – L298N flywheels + BTS7960 indexer
// Flash:   pio run -e test_all_motors -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Runs the complete firing sequence used by the main firmware, without the
// servo/LiDAR scan.  Useful for validating motor wiring and tuning duty
// cycles / timing independently of the aiming subsystem.
//
// Sequence:
//   t=0.0 s  Flywheels kick-start at 120/255 (overcome static friction)
//   t=0.3 s  Flywheels settle to FLY_DUTY (~20%)
//   t=5.3 s  Indexer ON at IDX_DUTY
//   t=55.3 s All motors stop
//
// Wiring:
//   L298N (Flywheel A & B):
//     EN_A → GPIO13   IN1 → GPIO14   IN2 → GPIO16
//     EN_B → GPIO27   IN3 → GPIO17   IN4 → GPIO23
//   BTS7960 (Indexer):
//     RPWM → GPIO18   LPWM → GPIO19
//     R_EN → GPIO25   L_EN → GPIO26
//
// Course concepts: multi-channel PWM via LEDC, H-bridge direction control,
//                  sequenced motor activation, kick-start to overcome stiction

#include <Arduino.h>

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
static constexpr int LEDC_CH_ENA      = 0;  // L298N Motor A
static constexpr int LEDC_CH_ENB      = 1;  // L298N Motor B
static constexpr int LEDC_CH_IDX      = 2;  // BTS7960 indexer

static constexpr int L298N_FREQ_HZ    = 1000;
static constexpr int BTS_FREQ_HZ      = 20000;
static constexpr int PWM_RES_BITS     = 8;

static constexpr uint8_t FLY_KICK_DUTY = 120; // brief high-duty pulse to overcome stiction
static constexpr uint8_t FLY_DUTY      = 42;  // ~20% of 255 – steady-state speed
static constexpr uint8_t IDX_DUTY      = 150;
static constexpr int     FLY_KICK_MS   = 300;
static constexpr int     FLY_SETTLE_MS = 5000;
static constexpr int     IDX_SPIN_MS   = 50000;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  All-motors sequenced test");
    Serial.printf("  L298N: ENA=GPIO%d  ENB=GPIO%d\n", PIN_EN_A, PIN_EN_B);
    Serial.printf("  BTS:   RPWM=GPIO%d LPWM=GPIO%d\n", PIN_IDX_RPWM, PIN_IDX_LPWM);
    Serial.println("─────────────────────────────────────\n");

    // ── L298N direction pins (both motors forward) ────────────────────────────
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);

    // ── L298N PWM setup ──────────────────────────────────────────────────────
    ledcSetup(LEDC_CH_ENA, L298N_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_EN_A, LEDC_CH_ENA);
    ledcSetup(LEDC_CH_ENB, L298N_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_EN_B, LEDC_CH_ENB);

    // ── BTS7960 setup ────────────────────────────────────────────────────────
    pinMode(PIN_IDX_R_EN, OUTPUT);
    pinMode(PIN_IDX_L_EN, OUTPUT);
    digitalWrite(PIN_IDX_R_EN, HIGH);
    digitalWrite(PIN_IDX_L_EN, HIGH);
    ledcSetup(LEDC_CH_IDX, BTS_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_IDX_RPWM, LEDC_CH_IDX);
    pinMode(PIN_IDX_LPWM, OUTPUT);
    digitalWrite(PIN_IDX_LPWM, LOW);

    // ── Step 1: kick-start flywheels then settle to steady-state ─────────────
    ledcWrite(LEDC_CH_ENA, FLY_KICK_DUTY);
    ledcWrite(LEDC_CH_ENB, FLY_KICK_DUTY);
    ledcWrite(LEDC_CH_IDX, 0);
    Serial.printf("t=0.0s  Flywheels kick-start at %d/255.\n", FLY_KICK_DUTY);
    delay(FLY_KICK_MS);

    ledcWrite(LEDC_CH_ENA, FLY_DUTY);
    ledcWrite(LEDC_CH_ENB, FLY_DUTY);
    Serial.printf("t=0.3s  Flywheels settled to %d/255. Indexer OFF.\n", FLY_DUTY);
    delay(FLY_SETTLE_MS);

    // ── Step 2: indexer on, flywheels still running ──────────────────────────
    Serial.printf("t=5.3s  Indexer ON at duty=%d\n", IDX_DUTY);
    ledcWrite(LEDC_CH_IDX, IDX_DUTY);
    delay(IDX_SPIN_MS);

    // ── Step 3: stop everything ──────────────────────────────────────────────
    ledcWrite(LEDC_CH_ENA, 0);
    ledcWrite(LEDC_CH_ENB, 0);
    ledcWrite(LEDC_CH_IDX, 0);
    Serial.println("t=55.3s All motors stopped.");
}

void loop() {
    // one-shot – test is done
}
