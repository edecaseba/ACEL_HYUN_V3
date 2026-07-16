#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "mock_arduino.h"
#include "EEPROM.h"
#include "motor_types.h"

static uint8_t mock_eeprom[1024];
static uint16_t mock_analog_read_value = 0;
static bool mock_safe_state_called = false;
static unsigned long mock_millis_value = 0;
static unsigned long mock_micros_value = 0;

void mock_eeprom_init(void) {
    memset(mock_eeprom, 0xFF, sizeof(mock_eeprom));
}

uint8_t EEPROMClass::read(int address) {
    if (address < 0 || address >= 1024) return 0xFF;
    return mock_eeprom[address];
}

void EEPROMClass::write(int address, uint8_t value) {
    if (address < 0 || address >= 1024) return;
    mock_eeprom[address] = value;
}

void EEPROMClass::update(int address, uint8_t value) {
    if (address < 0 || address >= 1024) return;
    mock_eeprom[address] = value;
}

int analogRead(uint8_t pin) {
    (void)pin;
    return mock_analog_read_value;
}

unsigned long millis(void) {
    return mock_millis_value;
}

unsigned long micros(void) {
    return mock_micros_value;
}

void safeState(void) {
    mock_safe_state_called = true;
}

// Mock for motor direction (needed by oc_calibrate)
ActuatorDirection sysState_currentDirection = ActuatorDirection::STOP;

#define F(x) x

#include "../src/overcurrent.cpp"

void test_oc_load_calibration_defaults_when_eeprom_empty(void) {
    oc_loadCalibration();
    TEST_ASSERT_EQUAL_UINT16(0, oc_getNominal());
    TEST_ASSERT_EQUAL_UINT16(4, oc_getSigma());
}

void test_oc_load_calibration_reads_valid_values(void) {
    EEPROM.update(EE_NOMINAL_ADDR, 500 & 0xFF);
    EEPROM.update(EE_NOMINAL_ADDR + 1, 500 >> 8);
    EEPROM.update(EE_SIGMA_ADDR, 10 & 0xFF);
    EEPROM.update(EE_SIGMA_ADDR + 1, 10 >> 8);

    oc_loadCalibration();
    TEST_ASSERT_EQUAL_UINT16(500, oc_getNominal());
    TEST_ASSERT_EQUAL_UINT16(10, oc_getSigma());
}

void test_oc_load_calibration_handles_out_of_range(void) {
    EEPROM.update(EE_NOMINAL_ADDR, 0xFF);
    EEPROM.update(EE_NOMINAL_ADDR + 1, 0xFF);
    EEPROM.update(EE_SIGMA_ADDR, 0xFF);
    EEPROM.update(EE_SIGMA_ADDR + 1, 0xFF);

    oc_loadCalibration();
    TEST_ASSERT_EQUAL_UINT16(0, oc_getNominal());
    TEST_ASSERT_EQUAL_UINT16(4, oc_getSigma());
}

void test_oc_save_calibration_writes_to_eeprom(void) {
    oc_saveCalibration(600, 15);

    TEST_ASSERT_EQUAL_UINT8(600 & 0xFF, EEPROM.read(EE_NOMINAL_ADDR));
    TEST_ASSERT_EQUAL_UINT8(600 >> 8, EEPROM.read(EE_NOMINAL_ADDR + 1));
    TEST_ASSERT_EQUAL_UINT8(15 & 0xFF, EEPROM.read(EE_SIGMA_ADDR));
    TEST_ASSERT_EQUAL_UINT8(15 >> 8, EEPROM.read(EE_SIGMA_ADDR + 1));
}

void test_oc_save_calibration_only_writes_when_changed(void) {
    EEPROM.update(EE_NOMINAL_ADDR, 600 & 0xFF);
    EEPROM.update(EE_NOMINAL_ADDR + 1, 600 >> 8);
    EEPROM.update(EE_SIGMA_ADDR, 15 & 0xFF);
    EEPROM.update(EE_SIGMA_ADDR + 1, 15 >> 8);

    oc_saveCalibration(600, 15);

    TEST_ASSERT_EQUAL_UINT8(600 & 0xFF, EEPROM.read(EE_NOMINAL_ADDR));
    TEST_ASSERT_EQUAL_UINT8(600 >> 8, EEPROM.read(EE_NOMINAL_ADDR + 1));
    TEST_ASSERT_EQUAL_UINT8(15 & 0xFF, EEPROM.read(EE_SIGMA_ADDR));
    TEST_ASSERT_EQUAL_UINT8(15 >> 8, EEPROM.read(EE_SIGMA_ADDR + 1));
}

void test_oc_calibrate_calculates_mean_and_sigma(void) {
    mock_analog_read_value = 512;

    oc_calibrate();
    for (int i = 0; i < OC_SAMPLES; i++) {
        mock_micros_value += 2000;
        oc_updateCalibration();
    }

    TEST_ASSERT_EQUAL_UINT16(512, oc_getNominal());
    TEST_ASSERT_EQUAL_UINT16(0, oc_getSigma());
}

void test_oc_calibrate_with_variance(void) {
    int sum = 0;
    for (int i = 0; i < OC_SAMPLES; i++) {
        mock_analog_read_value = 500 + (i % 2) * 20;
        sum += mock_analog_read_value;
        if (i == 0) {
            oc_calibrate();
        }
        mock_micros_value += 2000;
        oc_updateCalibration();
    }

    uint16_t expected_mean = sum / OC_SAMPLES;
    TEST_ASSERT_EQUAL_UINT16(expected_mean, oc_getNominal());
    TEST_ASSERT_GREATER_THAN_UINT16(0, oc_getSigma());
}

void test_oc_check_over_current_below_threshold(void) {
    oc_setNominalForTest(500);
    oc_setSigmaForTest(10);
    mock_analog_read_value = 949;  // below threshold 950

    bool result = oc_checkOverCurrentWithRaw(mock_analog_read_value);
    TEST_ASSERT_FALSE(result);
}

void test_oc_check_over_current_above_threshold(void) {
    oc_setNominalForTest(500);
    oc_setSigmaForTest(10);
    mock_analog_read_value = 951;  // above threshold 950

    bool result = oc_checkOverCurrentWithRaw(mock_analog_read_value);
    TEST_ASSERT_TRUE(result);
}

void test_oc_check_over_current_exact_threshold(void) {
    oc_setNominalForTest(500);
    oc_setSigmaForTest(10);
    mock_analog_read_value = 950;  // exactly at threshold 950

    bool result = oc_checkOverCurrentWithRaw(mock_analog_read_value);
    TEST_ASSERT_FALSE(result);
}

void test_oc_check_over_current_calls_safe_state(void) {
    oc_setNominalForTest(500);
    oc_setSigmaForTest(10);
    mock_analog_read_value = 951;  // above threshold 950

    oc_checkOverCurrent();

    TEST_ASSERT_TRUE(mock_safe_state_called);
}

void run_overcurrent_tests(void) {
    mock_eeprom_init();
    mock_analog_read_value = 0;
    mock_safe_state_called = false;
    mock_millis_value = 0;
    mock_micros_value = 0;
    oc_setNominalForTest(0);
    oc_setSigmaForTest(0);
    ocCalState = OC_IDLE;
    ocSampleIndex = 0;
    ocSum = 0;
    ocSumSq = 0;
    ocLastSampleTime = 0;

    RUN_TEST(test_oc_load_calibration_defaults_when_eeprom_empty);
    RUN_TEST(test_oc_load_calibration_reads_valid_values);
    RUN_TEST(test_oc_load_calibration_handles_out_of_range);
    RUN_TEST(test_oc_save_calibration_writes_to_eeprom);
    RUN_TEST(test_oc_save_calibration_only_writes_when_changed);
    RUN_TEST(test_oc_calibrate_calculates_mean_and_sigma);
    RUN_TEST(test_oc_calibrate_with_variance);
    RUN_TEST(test_oc_check_over_current_below_threshold);
    RUN_TEST(test_oc_check_over_current_above_threshold);
    RUN_TEST(test_oc_check_over_current_exact_threshold);
    RUN_TEST(test_oc_check_over_current_calls_safe_state);
}