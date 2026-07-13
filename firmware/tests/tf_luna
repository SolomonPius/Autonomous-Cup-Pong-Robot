// TF-Luna I2C LiDAR distance sensor test
// Flash:   pio run -e test_tfluna -t upload
// Monitor: pio device monitor          (115200 baud)
//
// Continuously reads the TF-Luna distance sensor over I2C (address 0x10) at
// 400 kHz fast-mode and prints distance, signal strength (flux), and internal
// temperature.  Each reading fetches a 6-byte frame from register 0x00:
//   bytes 0-1: distance (cm, little-endian uint16)
//   bytes 2-3: signal strength / flux (uint16)
//   bytes 4-5: chip temperature (°C × 10, int16)
//
// Expected output (one line per reading):
//   dist: 142 cm  flux: 1823  temp: 38.0 C
//
// If you see "ERROR: no ACK" the sensor is not responding –
// check SDA/SCL wiring and that 5 V is connected to TF-Luna pin 1.
//
// Wiring:
//   TF-Luna SDA → GPIO21
//   TF-Luna SCL → GPIO22
//   TF-Luna VCC → 5 V
//   TF-Luna GND → GND
//
// Course concepts: I2C protocol (register read with repeated-start),
//                  little-endian byte reconstruction, sensor data parsing

#include <Arduino.h>
#include <Wire.h>

static constexpr uint8_t  TFLUNA_ADDR = 0x10;
static constexpr uint8_t  REG_DIST_L  = 0x00;
static constexpr uint8_t  NUM_BYTES   = 6;

static constexpr int PIN_SDA = 21;
static constexpr int PIN_SCL = 22;

// Returns false and prints an error if the sensor does not ACK.
static bool luna_check() {
    Wire.beginTransmission(TFLUNA_ADDR);
    return Wire.endTransmission() == 0;
}

// Reads one 6-byte frame and prints it. Returns false on I2C error.
static bool luna_read_and_print() {
    Wire.beginTransmission(TFLUNA_ADDR);
    Wire.write(REG_DIST_L);
    if (Wire.endTransmission(false) != 0) return false;     // repeated-start
    if (Wire.requestFrom((uint8_t)TFLUNA_ADDR, NUM_BYTES) != NUM_BYTES) return false;

    uint8_t buf[NUM_BYTES];
    for (int i = 0; i < NUM_BYTES; i++) buf[i] = Wire.read();

    uint16_t dist_cm = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    uint16_t flux    = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
    int16_t  raw_t   = (int16_t) buf[4] | ((int16_t) buf[5] << 8);
    float    temp_c  = raw_t / 10.0f;

    const char *quality = "";
    if (flux < 100)   quality = "  [weak signal]";
    if (flux > 65000) quality = "  [saturated]";

    Serial.printf("dist: %4u cm  |  flux: %5u  |  temp: %.1f C%s\n",
                  dist_cm, flux, temp_c, quality);
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("─────────────────────────────────────");
    Serial.println("  TF-Luna I2C test");
    Serial.printf("  SDA=GPIO%d  SCL=GPIO%d  addr=0x%02X\n", PIN_SDA, PIN_SCL, TFLUNA_ADDR);
    Serial.println("─────────────────────────────────────");

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);                   // 400 kHz I2C fast-mode

    if (!luna_check()) {
        Serial.println("ERROR: no ACK from TF-Luna (addr 0x10).");
        Serial.println("  • Is 5V connected to TF-Luna pin 1?");
        Serial.println("  • SDA → GPIO21, SCL → GPIO22?");
        Serial.println("  • GND connected (TF-Luna pin 4)?");
        while (true) delay(1000);            // halt
    }

    Serial.println("TF-Luna found. Reading every 200 ms...\n");
}

void loop() {
    if (!luna_read_and_print()) {
        Serial.println("ERROR: I2C read failed.");
    }
    delay(200);
}
