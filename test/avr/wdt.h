#ifndef MOCK_AVR_WDT_H
#define MOCK_AVR_WDT_H

#define WDTO_15MS   0
#define WDTO_30MS   1
#define WDTO_60MS   2
#define WDTO_120MS  3
#define WDTO_250MS  4
#define WDTO_500MS  5
#define WDTO_1S     6
#define WDTO_2S     7
#define WDTO_4S     8
#define WDTO_8S     9

static inline void wdt_enable(uint8_t timeout) { (void)timeout; }
static inline void wdt_disable(void) {}
static inline void wdt_reset(void) {}

#endif