#include "overcurrent.h"
#include <math.h>

extern void safeState(); // forward declaration

static uint16_t g_nominal = 0;
static uint16_t g_sigma   = 0;

void oc_loadCalibration()
{
    uint16_t n = EEPROM.read(EE_NOMINAL_ADDR) |
                 (EEPROM.read(EE_NOMINAL_ADDR + 1) << 8);
    uint16_t s = EEPROM.read(EE_SIGMA_ADDR) |
                 (EEPROM.read(EE_SIGMA_ADDR + 1) << 8);
    // If EEPROM is erased (0xFFFF) use safe defaults
    g_nominal = (n == 0xFFFF) ? 0 : n;
    g_sigma   = (s == 0xFFFF) ? 4 : s;   // small sigma -> low threshold (conservative)
}

void oc_saveCalibration(uint16_t nominal, uint16_t sigma)
{
    uint16_t n = EEPROM.read(EE_NOMINAL_ADDR) |
                 (EEPROM.read(EE_NOMINAL_ADDR + 1) << 8);
    uint16_t s = EEPROM.read(EE_SIGMA_ADDR) |
                 (EEPROM.read(EE_SIGMA_ADDR + 1) << 8);
    if (n != nominal || s != sigma) {
        EEPROM.update(EE_NOMINAL_ADDR,     nominal & 0xFF);
        EEPROM.update(EE_NOMINAL_ADDR + 1, nominal >> 8);
        EEPROM.update(EE_SIGMA_ADDR,       sigma & 0xFF);
        EEPROM.update(EE_SIGMA_ADDR + 1,   sigma >> 8);
    }
}

void oc_calibrate()
{
    uint32_t sum = 0;
    uint32_t sumSq = 0;
    for (uint16_t i = 0; i < OC_SAMPLES; ++i) {
        uint16_t v = analogRead(OC_PIN);   // 0‑1023
        sum  += v;
        sumSq += ((uint32_t)v * v);
        // Non-blocking wait of approximately 2 ms
        unsigned long start = micros();
        while (micros() - start < 2000) {
            // Busy-wait; could add yield() if available
        }
    }
    uint16_t nominal = (uint16_t)(sum / OC_SAMPLES);
    uint32_t meanSq = ((uint32_t)nominal * nominal);
    int32_t variance = static_cast<int32_t>(sumSq / OC_SAMPLES) - static_cast<int32_t>(meanSq);
    if (variance < 0) variance = 0;
    if (variance > 65535) variance = 65535;
    uint16_t sigma = (uint16_t)sqrt(static_cast<double>(variance));

    oc_saveCalibration(nominal, sigma);
    g_nominal = nominal;
    g_sigma   = sigma;

    // Debug output (optional)
    Serial.print(F("[OC] Calibrated nominal="));
    Serial.print(g_nominal);
    Serial.print(F(" sigma="));
    Serial.println(g_sigma);
}

bool oc_checkOverCurrent()
{
    uint16_t raw = analogRead(OC_PIN);
    uint16_t threshold = g_nominal + (OC_K_SIGMA * g_sigma);
    if (raw > threshold) {
        safeState();                         // existing routine (PWM=0, EN=LOW)
        // Rate‑limit serial messages
        static uint32_t lastMsg = 0;
        if (millis() - lastMsg > 200) {
            Serial.println(F("[OC] Over‑current detected!"));
            lastMsg = millis();
        }
        return true;
    }
    return false;
}

uint16_t oc_getNominal() { return g_nominal; }
uint16_t oc_getSigma()   { return g_sigma; }

void oc_setNominalForTest(uint16_t value) { g_nominal = value; }
void oc_setSigmaForTest(uint16_t value)   { g_sigma   = value; }

bool oc_checkOverCurrentWithRaw(uint16_t raw)
{
    uint16_t threshold = g_nominal + (OC_K_SIGMA * g_sigma);
    if (raw > threshold) {
        // In test we don't call safeState to avoid side effects
        return true;
    }
    return false;
}