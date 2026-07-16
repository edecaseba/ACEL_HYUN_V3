#ifndef OVERCURRENT_H
#define OVERCURRENT_H

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

// Forward declaration - defined in main.cpp
enum class ActuatorDirection : uint8_t;

constexpr uint8_t OC_PIN            = A2;
constexpr uint16_t OC_SAMPLES       = 64;
constexpr uint16_t OC_K_SIGMA       = 0;
constexpr uint16_t OC_THRESHOLD_FIXED = 850;
constexpr uint16_t EE_NOMINAL_ADDR  = 0x00;
constexpr uint16_t EE_SIGMA_ADDR    = 0x02;
constexpr uint16_t OC_MIN_THRESHOLD = 950;

void oc_loadCalibration();
void oc_saveCalibration(uint16_t nominal, uint16_t sigma);
void oc_calibrate();
void oc_updateCalibration();
bool oc_checkOverCurrent();
uint16_t oc_getNominal();
uint16_t oc_getSigma();
bool oc_isCalibrated();
void oc_forceRecalibrate();

// For unit testing only
void oc_setNominalForTest(uint16_t value);
void oc_setSigmaForTest(uint16_t value);
bool oc_checkOverCurrentWithRaw(uint16_t raw);

#endif // OVERCURRENT_H