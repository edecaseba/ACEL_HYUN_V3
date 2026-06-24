/**
 * @brief Controlador PID en tiempo discreto — algoritmo puro extraído
 *
 * Proposito: algoritmo PID reutilizable entre main.cpp y tests unitarios.
 * Zero Dynamic RAM: solo tipos fijos, sin alloc/delete.
 * Framework-agnostic: no depende de Arduino.h, solo de <stdint.h> y <stdlib.h>.
 *
 * MCU target: ATmega328P (Arduino Nano) / ESP32-S3
 * Framework: agnostico
 *
 * v2.0: KP/KI/KD pasan de constexpr a parámetros runtime en PidInput.
 *        Se eliminan las constantes globales KP, KD, KI.
 */
#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

// ==========================================
// --- Constantes del algoritmo (fijas) ---
// ==========================================
constexpr float INTEGRAL_LIMIT     = 50.0f;
constexpr uint8_t ZONA_MUERTA      = 3;
constexpr uint8_t PWM_MIN_OPERACION = 90;

// ==========================================
// --- Estructuras de datos ---
// ==========================================

/**
 * @brief Entrada del controlador PID
 *
 * @param errorActual      Error actual (setpoint - posicion)
 * @param errorAnterior    Error de la iteracion anterior
 * @param integralAccumulator  Acumulador integral (entrada/salida)
 * @param Ts               Tiempo de muestreo en segundos (> 0)
 * @param kp               Ganancia proporcional (runtime, desde EEPROM)
 * @param ki               Ganancia integral (runtime, desde EEPROM)
 * @param kd               Ganancia derivativa (runtime, desde EEPROM)
 */
struct PidInput {
    int16_t errorActual;
    int16_t errorAnterior;
    float   integralAccumulator;
    float   Ts;
    float   kp;
    float   ki;
    float   kd;
};

/**
 * @brief Salida del controlador PID
 *
 * @param integralAccumulator  Acumulador integral actualizado
 * @param errorAnterior        errorActual (para realimentar en proxima llamada)
 * @param vel                  Velocidad PWM (0-255)
 * @param deberiaDetener       true si |error| <= ZONA_MUERTA
 * @param salidaFloat          Valor real del PID sin clamp
 */
struct PidOutput {
    float   integralAccumulator;
    int16_t errorAnterior;
    uint8_t vel;
    bool    deberiaDetener;
    float   salidaFloat;
};

// ==========================================
// --- Funciones auxiliares ---
// ==========================================

/**
 * @brief Verifica si el error esta dentro de la zona muerta
 * @param error [any int16_t]
 * @return true si |error| <= ZONA_MUERTA
 */
inline bool inDeadZone(int16_t error) {
    return (abs(static_cast<int>(error)) <= static_cast<int16_t>(ZONA_MUERTA));
}

/**
 * @brief Satura un valor float entre lo y hi
 * @param val Valor a saturar
 * @param lo  Limite inferior
 * @param hi  Limite superior (debe ser >= lo)
 * @return val acotado en [lo, hi]
 */
inline float clampFloat(float val, float lo, float hi) {
    if (val < lo) {
        return lo;
    }
    if (val > hi) {
        return hi;
    }
    return val;
}

// ==========================================
// --- Algoritmo PID principal ---
// ==========================================

/**
 * @brief Computa una iteracion del controlador PID
 *
 * Formula: u = Kp * e + Kd * (e - e_prev) / Ts + integral
 * El termino derivativo se normaliza por Ts para mantener la ganancia
 * efectiva constante independientemente del tiempo de muestreo.
 *
 * Anti-windup: integral satura en [-INTEGRAL_LIMIT, +INTEGRAL_LIMIT]
 * Zona muerta: si |error| <= ZONA_MUERTA, deberiaDetener = true
 *
 * KP/KI/KD se toman de los campos kp/ki/kd de PidInput (valores runtime)
 *
 * @param in  [in]  Parametros de entrada (incluye kp, ki, kd)
 * @param out [out] Resultados del PID
 */
inline void pidCompute(const PidInput& in, PidOutput& out) {
    out.deberiaDetener = false;
    out.integralAccumulator = in.integralAccumulator;

    // --- Zona muerta: no acumular integral, no mover ---
    if (inDeadZone(in.errorActual)) {
        out.deberiaDetener = true;
        out.salidaFloat = 0.0f;
        out.vel = 0;
        out.errorAnterior = in.errorActual;
        return;
    }

    // --- Acumulacion integral con anti-windup ---
    out.integralAccumulator += static_cast<float>(in.errorActual) * in.ki * in.Ts;

    out.integralAccumulator = clampFloat(out.integralAccumulator, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    // --- Termino derivativo normalizado por Ts ---
    float derivada = static_cast<float>(in.errorActual - in.errorAnterior);
    derivada /= in.Ts;

    // --- Salida PID completa ---
    out.salidaFloat = (static_cast<float>(in.errorActual) * in.kp)
                    + (derivada * in.kd)
                    + out.integralAccumulator;

    // --- Clamp de velocidad PWM ---
    float absSalida = fabsf(out.salidaFloat);

    if (absSalida < static_cast<float>(PWM_MIN_OPERACION)) {
        absSalida = static_cast<float>(PWM_MIN_OPERACION);
    } else if (absSalida > 255.0f) {
        absSalida = 255.0f;
    }
    out.vel = static_cast<uint8_t>(absSalida);

    // --- Actualizar error anterior ---
    out.errorAnterior = in.errorActual;
}

#endif /* PID_CONTROLLER_H */
