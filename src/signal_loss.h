/**
 * @brief Chequeo de plausibilidad de sensores potenciometricos — algoritmo puro
 *
 * El actuador NO tiene finales de carrera fisicos. El unico limite de
 * posicion es el rango calibrado del potenciometro (pMin/pMax, mMin/mMax)
 * mas el stall por sobrecorriente contra el tope mecanico real (ver
 * overcurrent.h). Si un potenciometro pierde señal (cable cortado, wiper
 * desconectado — divisor resistivo simple, sin pull-up/pull-down: el wiper
 * puede flotar a cualquier valor, no a un riel conocido), la unica defensa
 * es notar que la lectura queda fuera del rango fisico medido en calibracion.
 *
 * Framework-agnostic: no depende de Arduino.h, solo de <stdint.h>.
 */
#ifndef SIGNAL_LOSS_H
#define SIGNAL_LOSS_H

#include <stdint.h>

constexpr uint16_t SIGNAL_LOSS_MARGIN_ADC = 50;
constexpr uint16_t SIGNAL_LOSS_TIMEOUT_MS = 50;
constexpr int16_t  ADC_MAX_10BIT          = 1023;

/**
 * @brief Indica si una lectura ADC cruda esta fuera del rango calibrado
 *        (con margen de tolerancia) para un potenciometro dado.
 * @param raw   Lectura cruda de analogRead() (0-1023)
 * @param calA  Un extremo calibrado (pMin o mMin — puede ser mayor o menor que calB)
 * @param calB  El otro extremo calibrado (pMax o mMax)
 * @return true si `raw` esta fuera de [min(calA,calB)-margen, max(calA,calB)+margen]
 */
inline bool fueraDeRangoPlausible(uint16_t raw, int16_t calA, int16_t calB) {
    int16_t lo = (calA < calB) ? calA : calB;
    int16_t hi = (calA < calB) ? calB : calA;
    lo = static_cast<int16_t>(lo - static_cast<int16_t>(SIGNAL_LOSS_MARGIN_ADC));
    hi = static_cast<int16_t>(hi + static_cast<int16_t>(SIGNAL_LOSS_MARGIN_ADC));
    if (lo < 0)              { lo = 0; }
    if (hi > ADC_MAX_10BIT)  { hi = ADC_MAX_10BIT; }
    return (static_cast<int16_t>(raw) < lo) || (static_cast<int16_t>(raw) > hi);
}

#endif /* SIGNAL_LOSS_H */
