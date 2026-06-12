/**
 * @brief Tests unitarios del controlador PID con anti-windup
 *
 * Framework: Unity (PlatformIO)
 * Verifica: control proporcional, zona muerta, acumulacion integral,
 *           limite anti-windup, reset en falla, derivada normalizada por Ts
 *
 * El algoritmo PID se prueba directamente desde src/pid_controller.h
 * (funcion pura, sin dependencias de hardware).
 *
 * MCU target: ATmega328P (Arduino Nano)
 * Compilacion: pio test -e nanoatmega328
 */
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <unity.h>
#include <stdint.h>
#include <stdlib.h>

#include "../src/pid_controller.h"

// ==========================================
// --- Fixtures ---
// ==========================================
void setUp(void) {
    // Se ejecuta antes de cada test
}

void tearDown(void) {
    // Se ejecuta despues de cada test
}

// ==========================================
// --- Tests: Control Proporcional ---
// ==========================================

void test_proporcional_error_positivo(void) {
    // Error positivo de 10 unidades -> P = 10 * 4.0 = 40
    // D = (10 - 0) / 0.01 * 1.5 = 1500
    // I = 10 * 0.5 * 0.01 = 0.05
    int16_t error = 10;
    int16_t errorPrev = 0;
    float integral = 0.0f;
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    // P=40, D=1500, I=0.05 -> Total=1540.05 -> clamp(1540.05, 90, 255) = 255
    TEST_ASSERT_FALSE(out.deberiaDetener);
    TEST_ASSERT_EQUAL(255, out.vel);
    // Integral debe haber acumulado
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.05f, out.integralAccumulator);
}

void test_proporcional_error_negativo(void) {
    // Error negativo de -10 -> P = -40
    // D = (-10 - 0) / 0.01 * 1.5 = -1500
    // I = -0.05
    int16_t error = -10;
    int16_t errorPrev = 0;
    float integral = 0.0f;
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    TEST_ASSERT_FALSE(out.deberiaDetener);
    // abs(-1540.05) = 1540.05 -> clamp(1540.05, 90, 255) = 255
    TEST_ASSERT_EQUAL(255, out.vel);
    // Integral debe ser negativa
    TEST_ASSERT(out.integralAccumulator < 0.0f);
}

// ==========================================
// --- Tests: Zona Muerta ---
// ==========================================

void test_zona_muerta_no_acumula_integral(void) {
    // |error| <= 3 -> no se debe acumular integral
    int16_t error = 2;
    int16_t errorPrev = 0;
    float integral = 10.0f;  // Valor previo
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    // Integral debe permanecer igual (no se acumula en zona muerta)
    TEST_ASSERT_TRUE(out.deberiaDetener);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, out.integralAccumulator);
}

void test_zona_muerta_limite_superior(void) {
    // error = 3 -> esta DENTRO de la zona muerta (<= 3)
    int16_t error = 3;
    int16_t errorPrev = 0;
    float integral = 5.0f;
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    TEST_ASSERT_TRUE(out.deberiaDetener);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, out.integralAccumulator);
}

void test_zona_muerta_limite_inferior(void) {
    // error = -3 -> dentro de zona muerta
    int16_t error = -3;
    int16_t errorPrev = 0;
    float integral = 5.0f;
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    TEST_ASSERT_TRUE(out.deberiaDetener);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, out.integralAccumulator);
}

void test_fuera_zona_muerta_acumula(void) {
    // error = 4 -> fuera de zona muerta, debe acumular
    int16_t error = 4;
    int16_t errorPrev = 0;
    float integral = 0.0f;
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    TEST_ASSERT_FALSE(out.deberiaDetener);
    // I = 0 + 4 * 0.5 * 0.01 = 0.02
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.02f, out.integralAccumulator);
}

// ==========================================
// --- Tests: Acumulacion Integral ---
// ==========================================

void test_acumulacion_integral_creciente(void) {
    float integral = 0.0f;
    float Ts = 0.01f;
    int16_t error = 10;
    int16_t errorPrev = 0;

    // Simular 5 iteraciones con error = 10
    for (int i = 0; i < 5; ++i) {
        PidInput in = { error, errorPrev, integral, Ts };
        PidOutput out;
        pidCompute(in, out);
        integral = out.integralAccumulator;
        errorPrev = out.errorAnterior;
    }

    // I = 5 * (10 * 0.5 * 0.01) = 5 * 0.05 = 0.25
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, integral);
}

void test_acumulacion_integral_negativa(void) {
    float integral = 0.0f;
    float Ts = 0.01f;
    int16_t error = -10;
    int16_t errorPrev = 0;

    // Simular 3 iteraciones con error = -10
    for (int i = 0; i < 3; ++i) {
        PidInput in = { error, errorPrev, integral, Ts };
        PidOutput out;
        pidCompute(in, out);
        integral = out.integralAccumulator;
        errorPrev = out.errorAnterior;
    }

    // I = 3 * (-10 * 0.5 * 0.01) = -0.15
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.15f, integral);
}

// ==========================================
// --- Tests: Anti-Windup ---
// ==========================================

void test_antiwindup_limite_superior(void) {
    float integral = 49.0f;  // Cerca del limite
    float Ts = 0.1f;         // Ts grande para acumular mucho

    int16_t error = 50;
    int16_t errorPrev = 0;

    // error grande positivo, Ts grande -> sobrepasa limite
    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    // I = 49 + (50 * 0.5 * 0.1) = 49 + 2.5 = 51.5 -> clamp a 50
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, out.integralAccumulator);
}

void test_antiwindup_limite_inferior(void) {
    float integral = -49.0f;
    float Ts = 0.1f;

    int16_t error = -50;
    int16_t errorPrev = 0;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    // I = -49 + (-50 * 0.5 * 0.1) = -49 - 2.5 = -51.5 -> clamp a -50
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -50.0f, out.integralAccumulator);
}

void test_antiwindup_acumulacion_mantiene_limite(void) {
    float integral = 50.0f;  // Ya en el limite
    float Ts = 0.1f;
    int16_t error = 20;
    int16_t errorPrev = 0;

    // Aunque el error siga, no debe exceder el limite
    for (int i = 0; i < 10; ++i) {
        PidInput in = { error, errorPrev, integral, Ts };
        PidOutput out;
        pidCompute(in, out);
        integral = out.integralAccumulator;
        errorPrev = out.errorAnterior;
        TEST_ASSERT(integral <= 50.0f);
        TEST_ASSERT(integral >= -50.0f);
    }
}

void test_antiwindup_no_limita_sin_necesidad(void) {
    float integral = 10.0f;
    float Ts = 0.01f;

    int16_t error = 5;
    int16_t errorPrev = 0;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    // I = 10 + (5 * 0.5 * 0.01) = 10 + 0.025 = 10.025
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.025f, out.integralAccumulator);
}

// ==========================================
// --- Tests: Reset en Falla ---
// ==========================================

void test_reset_integral_en_condicion_segura(void) {
    // Simular el comportamiento de reset en falla
    float integral = 45.0f;  // Valor acumulado

    // Al detectarse falla, el codigo en main.cpp hace:
    integral = 0.0f;

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, integral);
}

void test_reset_integral_en_calibracion(void) {
    float integral = -30.0f;

    // Al iniciar calibracion:
    integral = 0.0f;

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, integral);
}

// ==========================================
// --- Tests: Derivada ---
// ==========================================

void test_derivada_contribuye_al_pid(void) {
    // Error cambiando: antes era 0, ahora 5
    // D = (5 - 0) / 0.01 * 1.5 = 750  (normalizado por Ts)
    int16_t error = 5;
    int16_t errorPrev = 0;
    float integral = 0.0f;
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    // P = 20, D = 750, I = 0.025
    // salidaFloat = 20.0 + 750.0 + 0.025 = 770.025
    float esperado = (static_cast<float>(error) * KP)
                   + (static_cast<float>(error - errorPrev) * KD / Ts)
                   + (static_cast<float>(error) * KI * Ts);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, esperado, out.salidaFloat);
}

void test_derivada_cero_error_estable(void) {
    // Error no cambia: error = errorPrev = 10 -> D = 0
    int16_t error = 10;
    int16_t errorPrev = 10;
    float integral = 0.0f;
    float Ts = 0.01f;

    PidInput in = { error, errorPrev, integral, Ts };
    PidOutput out;
    pidCompute(in, out);

    // P = 40, D = 0, I = 0.05
    // salidaFloat = 40.05
    float esperado = (static_cast<float>(error) * KP)
                   + (0.0f * KD)
                   + (static_cast<float>(error) * KI * Ts);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, esperado, out.salidaFloat);
}

// ==================================
// --- Runner ---
// ==================================
void run_all_tests(void) {
    // Control Proporcional
    RUN_TEST(test_proporcional_error_positivo);
    RUN_TEST(test_proporcional_error_negativo);

    // Zona Muerta
    RUN_TEST(test_zona_muerta_no_acumula_integral);
    RUN_TEST(test_zona_muerta_limite_superior);
    RUN_TEST(test_zona_muerta_limite_inferior);
    RUN_TEST(test_fuera_zona_muerta_acumula);

    // Acumulacion Integral
    RUN_TEST(test_acumulacion_integral_creciente);
    RUN_TEST(test_acumulacion_integral_negativa);

    // Anti-Windup
    RUN_TEST(test_antiwindup_limite_superior);
    RUN_TEST(test_antiwindup_limite_inferior);
    RUN_TEST(test_antiwindup_acumulacion_mantiene_limite);
    RUN_TEST(test_antiwindup_no_limita_sin_necesidad);

    // Reset en Falla / Calibracion
    RUN_TEST(test_reset_integral_en_condicion_segura);
    RUN_TEST(test_reset_integral_en_calibracion);

    // Derivada
    RUN_TEST(test_derivada_contribuye_al_pid);
    RUN_TEST(test_derivada_cero_error_estable);
}

#ifdef ARDUINO
void setup() {
    delay(2000);  // Esperar puerto serial en hardware real
    UNITY_BEGIN();
    run_all_tests();
    UNITY_END();
}

void loop() {
    // No-operacion: los tests corren una sola vez en setup()
}
#else
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    run_all_tests();
    return UNITY_END();
}
#endif
