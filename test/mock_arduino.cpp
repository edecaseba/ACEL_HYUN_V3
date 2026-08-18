#include "mock_arduino.h"
#include <string.h>
#include <cstdio>

// Mock EEPROM
uint8_t mock_eeprom[1024];

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

// Global EEPROM instance for Arduino compatibility
EEPROMClass EEPROM;

// Mock analogRead
uint16_t mock_analog_read_value = 0;

int analogRead(uint8_t pin) {
    (void)pin;
    return mock_analog_read_value;
}

// Mock millis/micros
unsigned long mock_millis_value = 0;
unsigned long mock_micros_value = 0;

unsigned long millis(void) {
    return mock_millis_value;
}

unsigned long micros(void) {
    return mock_micros_value;
}

// Mock safeState
bool mock_safe_state_called = false;

void safeState(void) {
    mock_safe_state_called = true;
}

// Mock motor direction (needed by overcurrent.cpp)
ActuatorDirection sysState_currentDirection = ActuatorDirection::STOP;

// Mock Serial
char mock_serial_buffer[256] = {0};

void Serial_begin(unsigned long baud) {
    (void)baud;
}

void Serial_print(const char* str) {
    strncat(mock_serial_buffer, str, sizeof(mock_serial_buffer) - strlen(mock_serial_buffer) - 1);
}

void Serial_println(const char* str) {
    Serial_print(str);
    strncat(mock_serial_buffer, "\n", sizeof(mock_serial_buffer) - strlen(mock_serial_buffer) - 1);
}

void Serial_print_int(int val) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    Serial_print(buf);
}

void Serial_print_uint16(uint16_t val) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", val);
    Serial_print(buf);
}

// Serial instance
MockSerial Serial;