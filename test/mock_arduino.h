#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <stdint.h>
#include <Arduino.h>
#include "EEPROM.h"
#include "motor_types.h"
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif

// Mock EEPROM
extern uint8_t mock_eeprom[1024];
void mock_eeprom_init(void);

// Arduino function mocks
extern uint16_t mock_analog_read_value;
extern unsigned long mock_millis_value;
extern unsigned long mock_micros_value;

int analogRead(uint8_t pin);
unsigned long millis(void);
unsigned long micros(void);

// EEPROM class mocks
uint8_t EEPROMClass_read(int address);
void EEPROMClass_write(int address, uint8_t value);
void EEPROMClass_update(int address, uint8_t value);

// Safe state mock
extern bool mock_safe_state_called;
void safeState(void);

// Motor direction mock (for overcurrent)
extern ActuatorDirection sysState_currentDirection;

// Serial mock
extern char mock_serial_buffer[256];
void Serial_begin(unsigned long baud);
void Serial_print(const char* str);
void Serial_println(const char* str);
void Serial_print_int(int val);
void Serial_print_uint16(uint16_t val);

// F() macro mock
#define F(x) x

// Pin definitions for native test
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define A6 20
#define A7 21

#ifdef __cplusplus
}
#endif

// Global EEPROM instance for Arduino compatibility (C++ linkage)
extern EEPROMClass EEPROM;

// Serial class mock for Arduino compatibility
struct MockSerial {
    void begin(unsigned long baud) { Serial_begin(baud); }
    void print(const char* str) { Serial_print(str); }
    void println(const char* str) { Serial_println(str); }
    void print(int val) { Serial_print_int(val); }
    void print(uint16_t val) { Serial_print_uint16(val); }
    void println(int val) { Serial_print_int(val); Serial_println(""); }
    void println(uint16_t val) { Serial_print_uint16(val); Serial_println(""); }
    void print(float val, int decimals) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, val);
        Serial_print(buf);
    }
};

extern MockSerial Serial;

#endif // MOCK_ARDUINO_H