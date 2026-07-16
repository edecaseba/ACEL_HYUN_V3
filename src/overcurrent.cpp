#include "overcurrent.h"
#include <math.h>
#include <avr/wdt.h>

extern void safeState();
#include "motor_types.h"

extern ActuatorDirection sysState_currentDirection;

enum OcCalState { OC_IDLE, OC_SAMPLING };
static OcCalState ocCalState = OC_IDLE;
static uint16_t ocSampleIndex = 0;
static uint32_t ocSum = 0;
static uint32_t ocSumSq = 0;
static unsigned long ocLastSampleTime = 0;
static const uint16_t OC_SAMPLE_INTERVAL_US = 2000;

static uint16_t g_nominal = 0;
static uint16_t g_sigma   = 0;

void oc_loadCalibration()
{
    uint16_t n = static_cast<uint16_t>(EEPROM.read(EE_NOMINAL_ADDR)) |
                 (static_cast<uint16_t>(EEPROM.read(EE_NOMINAL_ADDR + 1)) << 8);
    uint16_t s = static_cast<uint16_t>(EEPROM.read(EE_SIGMA_ADDR)) |
                 (static_cast<uint16_t>(EEPROM.read(EE_SIGMA_ADDR + 1)) << 8);
    if (n == 0xFFFF || n > 1023) n = 0;
    if (s == 0xFFFF || s > 500)  s = 4;
    g_nominal = n;
    g_sigma   = s;
}

void oc_saveCalibration(uint16_t nominal, uint16_t sigma)
{
    uint16_t n = static_cast<uint16_t>(EEPROM.read(EE_NOMINAL_ADDR)) |
                 (static_cast<uint16_t>(EEPROM.read(EE_NOMINAL_ADDR + 1)) << 8);
    uint16_t s = static_cast<uint16_t>(EEPROM.read(EE_SIGMA_ADDR)) |
                 (static_cast<uint16_t>(EEPROM.read(EE_SIGMA_ADDR + 1)) << 8);
    if (n != nominal || s != sigma) {
        EEPROM.update(EE_NOMINAL_ADDR,     static_cast<uint8_t>(nominal & 0xFF));
        EEPROM.update(EE_NOMINAL_ADDR + 1, static_cast<uint8_t>(nominal >> 8));
        EEPROM.update(EE_SIGMA_ADDR,       static_cast<uint8_t>(sigma & 0xFF));
        EEPROM.update(EE_SIGMA_ADDR + 1,   static_cast<uint8_t>(sigma >> 8));
    }
}

void oc_calibrate()
{
    if (sysState_currentDirection != ActuatorDirection::STOP) {
        Serial.println(F("[OC] Error: Motor en movimiento. Detenga antes de calibrar."));
        return;
    }
    if (ocCalState == OC_IDLE) {
        ocCalState = OC_SAMPLING;
        ocSampleIndex = 0;
        ocSum = 0;
        ocSumSq = 0;
        ocLastSampleTime = micros();
        Serial.println(F("[OC] Calibration started..."));
    }
}

void oc_updateCalibration()
{
    if (ocCalState != OC_SAMPLING) return;

    unsigned long now = micros();
    if (now - ocLastSampleTime >= OC_SAMPLE_INTERVAL_US) {
        ocLastSampleTime = now;
        uint16_t v = static_cast<uint16_t>(analogRead(OC_PIN));
        ocSum += v;
        ocSumSq += static_cast<uint32_t>(v) * static_cast<uint32_t>(v);
        ocSampleIndex++;

        if (ocSampleIndex >= OC_SAMPLES) {
            uint16_t nominal = static_cast<uint16_t>(ocSum / OC_SAMPLES);
            uint32_t meanSq = static_cast<uint32_t>(nominal) * static_cast<uint32_t>(nominal);
            int32_t variance = static_cast<int32_t>(ocSumSq / OC_SAMPLES) - static_cast<int32_t>(meanSq);
            if (variance < 0) variance = 0;
            if (variance > 65535) variance = 65535;
            uint16_t sigma = static_cast<uint16_t>(sqrt(static_cast<double>(variance)) + 0.5);

            if (nominal == 0 || nominal >= 1023) {
                Serial.println(F("[OC] Error: Lectura invalida (nominal=0 o saturado). Verifique sensor A2."));
                ocCalState = OC_IDLE;
                g_nominal = 0;
                g_sigma = 0;
                return;
            }

            oc_saveCalibration(nominal, sigma);
            g_nominal = nominal;
            g_sigma   = sigma;

            ocCalState = OC_IDLE;
            Serial.print(F("[OC] Calibrated nominal="));
            Serial.print(g_nominal);
            Serial.print(F(" sigma="));
            Serial.println(g_sigma);
        }
    }
}

bool oc_checkOverCurrent()
{
    wdt_reset();
    uint16_t raw = static_cast<uint16_t>(analogRead(OC_PIN));
    uint16_t threshold = g_nominal + (OC_K_SIGMA * g_sigma);
    if (threshold < OC_MIN_THRESHOLD) threshold = OC_MIN_THRESHOLD;
    if (raw > threshold) {
        safeState();
        static uint32_t lastMsg = 0;
        if (millis() - lastMsg > 200) {
            Serial.print(F("[OC] Over-current! raw=")); Serial.print(raw);
            Serial.print(F(" thr=")); Serial.println(threshold);
            lastMsg = millis();
        }
        return true;
    }
    return false;
}

uint16_t oc_getNominal() { return g_nominal; }
uint16_t oc_getSigma()   { return g_sigma; }

bool oc_isCalibrated() { return ocCalState == OC_IDLE && g_nominal != 0; }

void oc_forceRecalibrate()
{
    ocCalState = OC_IDLE;
    g_nominal = 0;
    g_sigma = 0;
    oc_calibrate();
}

void oc_setNominalForTest(uint16_t value) { g_nominal = value; }
void oc_setSigmaForTest(uint16_t value)   { g_sigma   = value; }

bool oc_checkOverCurrentWithRaw(uint16_t raw)
{
    uint16_t threshold = g_nominal + (OC_K_SIGMA * g_sigma);
    if (threshold < OC_MIN_THRESHOLD) threshold = OC_MIN_THRESHOLD;
    return raw > threshold;
}