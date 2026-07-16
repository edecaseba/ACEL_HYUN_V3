#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19

#ifdef __cplusplus
extern "C" {
#endif

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

#define F(x) x

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class Serial_ {
public:
    void begin(unsigned long baud) { (void)baud; }
    void print(const char* s) { (void)s; }
    void println(const char* s) { (void)s; }
    void print(int n) { (void)n; }
    void println(int n) { (void)n; }
    void print(unsigned int n) { (void)n; }
    void println(unsigned int n) { (void)n; }
    void print(long n) { (void)n; }
    void println(long n) { (void)n; }
    void print(unsigned long n) { (void)n; }
    void println(unsigned long n) { (void)n; }
    void print(float f, int d) { (void)f; (void)d; }
    void println(float f, int d) { (void)f; (void)d; }
    int available() { return 0; }
    int read() { return -1; }
    size_t write(uint8_t c) { (void)c; return 1; }
};

extern Serial_ Serial;
#endif

#endif