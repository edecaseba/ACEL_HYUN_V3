#ifndef MOCK_EEPROM_H
#define MOCK_EEPROM_H

#include <stdint.h>

class EEPROMClass {
public:
    uint8_t read(int address);
    void write(int address, uint8_t value);
    void update(int address, uint8_t value);
    uint8_t operator[](int address) { return read(address); }
    int length() { return 1024; }
};

extern EEPROMClass EEPROM;

#endif