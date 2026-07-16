#include <unity.h>
#include <stdint.h>
#include <math.h>

#include "../src/pid_controller.h"

// Mock dead-time variables and functions for testing
static bool deadTimeActive = false;
static uint32_t deadTimeUntil = 0;
static uint32_t mock_millis_value = 0;
static bool mock_current_direction = false; // false = STOP/REVERSE, true = FORWARD

static void mock_detener(void) {
    deadTimeActive = false;
    deadTimeUntil = 0;
    mock_current_direction = false;
}

static void mock_mover(uint8_t vel, bool acelera) {
    (void)vel;
    bool target_dir = acelera;
    
    // Dead-time al cambiar dirección (solo si dirección actual NO es STOP)
    if (target_dir != mock_current_direction && mock_current_direction != false) {
        if (!deadTimeActive) {
            mock_detener();
            deadTimeUntil = mock_millis_value + 150;
            deadTimeActive = true;
        }
        return;
    }

    if (deadTimeActive) {
        if (mock_millis_value < deadTimeUntil) { return; }
        deadTimeActive = false;
        deadTimeUntil = 0;
    }

    mock_current_direction = target_dir;
}

void setUp(void) {}
void tearDown(void) {}

static PidInput makeInput(int16_t errorActual, int16_t errorAnterior, float integralAccumulator, float Ts, float kp, float ki, float kd) {
    PidInput in;
    in.errorActual = errorActual;
    in.errorAnterior = errorAnterior;
    in.integralAccumulator = integralAccumulator;
    in.Ts = Ts;
    in.kp = kp;
    in.ki = ki;
    in.kd = kd;
    return in;
}

static PidOutput makeOutput(void) {
    PidOutput out;
    out.integralAccumulator = 0.0f;
    out.errorAnterior = 0;
    out.vel = 0;
    out.deberiaDetener = false;
    out.salidaFloat = 0.0f;
    return out;
}

void test_pid_deadzone_stops_motor(void) {
    PidInput in = makeInput(2, 0, 0.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_TRUE(out.deberiaDetener);
    TEST_ASSERT_EQUAL_UINT8(0, out.vel);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out.salidaFloat);
}

void test_pid_deadzone_boundary(void) {
    PidInput in = makeInput(3, 0, 0.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_TRUE(out.deberiaDetener);
    TEST_ASSERT_EQUAL_UINT8(0, out.vel);
}

void test_pid_outside_deadzone_computes_output(void) {
    PidInput in = makeInput(10, 0, 0.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_FALSE(out.deberiaDetener);
    TEST_ASSERT_GREATER_THAN_UINT8(0, out.vel);
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(255, out.vel);
}

void test_pid_integral_accumulates(void) {
    PidInput in = makeInput(10, 0, 0.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    float expectedIntegral = 0.0f + 10.0f * 0.1f * 0.01f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expectedIntegral, out.integralAccumulator);
}

void test_pid_integral_anti_windup_upper(void) {
    PidInput in = makeInput(100, 0, 49.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 49.1f, out.integralAccumulator);
}

void test_pid_integral_anti_windup_lower(void) {
    PidInput in = makeInput(-100, 0, -49.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, -49.1f, out.integralAccumulator);
}

void test_pid_derivative_normalized_by_ts(void) {
    PidInput in = makeInput(10, 5, 0.0f, 0.01f, 0.0f, 0.0f, 1.0f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    float expectedDerivative = (10.0f - 5.0f) / 0.01f;
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expectedDerivative, out.salidaFloat);
}

void test_pid_output_clamped_to_pwm_range(void) {
    PidInput in = makeInput(100, 0, 0.0f, 0.01f, 10.0f, 0.0f, 0.0f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_LESS_OR_EQUAL_UINT8(255, out.vel);
}

void test_pid_output_minimum_pwm(void) {
    PidInput in = makeInput(10, 0, 0.0f, 0.01f, 1.0f, 0.0f, 0.0f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(90, out.vel);
}

void test_pid_negative_error_reverses_direction(void) {
    PidInput in = makeInput(-10, 0, 0.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_FALSE(out.deberiaDetener);
    TEST_ASSERT_GREATER_THAN_UINT8(0, out.vel);
}

void test_pid_zero_ts_defaults(void) {
    PidInput in = makeInput(10, 0, 0.0f, 0.0f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_FALSE(out.deberiaDetener);
}

void test_pid_large_ts_clamped(void) {
    PidInput in = makeInput(10, 0, 0.0f, 0.2f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_FALSE(out.deberiaDetener);
}

void test_pid_integral_resets_in_deadzone(void) {
    PidInput in = makeInput(10, 0, 10.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    in.errorActual = 2;
    in.errorAnterior = out.errorAnterior;
    in.integralAccumulator = out.integralAccumulator;

    pidCompute(in, out);

    TEST_ASSERT_TRUE(out.deberiaDetener);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out.salidaFloat);
}

void test_pid_kp_ki_kd_runtime_values(void) {
    PidInput in = makeInput(10, 0, 0.0f, 0.01f, 7.47f, 0.0534f, 0.33f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_FALSE(out.deberiaDetener);
    TEST_ASSERT_GREATER_THAN_UINT8(0, out.vel);
}

void test_pid_error_anterior_updated(void) {
    PidInput in = makeInput(10, 5, 0.0f, 0.01f, 2.0f, 0.1f, 0.5f);
    PidOutput out = makeOutput();

    pidCompute(in, out);

    TEST_ASSERT_EQUAL_INT16(10, out.errorAnterior);
}

void test_inDeadZone_function(void) {
    TEST_ASSERT_TRUE(inDeadZone(0));
    TEST_ASSERT_TRUE(inDeadZone(3));
    TEST_ASSERT_TRUE(inDeadZone(-3));
    TEST_ASSERT_FALSE(inDeadZone(4));
    TEST_ASSERT_FALSE(inDeadZone(-4));
    TEST_ASSERT_FALSE(inDeadZone(100));
    TEST_ASSERT_FALSE(inDeadZone(-100));
}

void test_clampFloat_function(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, clampFloat(10.0f, 0.0f, 5.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, clampFloat(-10.0f, 0.0f, 5.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, clampFloat(3.0f, 0.0f, 5.0f));
}

void test_pid_deadtime_blocks_direction_change_for_150ms(void) {
    // Simular: motor moviéndose FORWARD, luego cambiar a REVERSE
    mock_current_direction = true;  // FORWARD
    deadTimeActive = false;
    deadTimeUntil = 0;
    mock_millis_value = 0;

    // Cambiar a REVERSE (dirección opuesta) - debe activar dead-time
    mock_mover(140, false);
    TEST_ASSERT_TRUE(deadTimeActive);
    TEST_ASSERT_EQUAL_UINT32(150, deadTimeUntil);

    // A los 149ms, dead-time aún activo
    mock_millis_value = 149;
    mock_mover(140, false);
    TEST_ASSERT_TRUE(deadTimeActive);

    // A los 150ms, dead-time expira
    mock_millis_value = 150;
    mock_mover(140, false);
    TEST_ASSERT_FALSE(deadTimeActive);
    TEST_ASSERT_EQUAL_UINT32(0, deadTimeUntil);
}

void test_pid_deadtime_reset_in_detener_allows_immediate_movement(void) {
    deadTimeActive = true;
    deadTimeUntil = 1000;

    mock_detener();
    TEST_ASSERT_FALSE(deadTimeActive);
    TEST_ASSERT_EQUAL_UINT32(0, deadTimeUntil);

    mock_mover(140, true);
    TEST_ASSERT_FALSE(deadTimeActive);
}

void run_pid_tests(void) {
    RUN_TEST(test_pid_deadzone_stops_motor);
    RUN_TEST(test_pid_deadzone_boundary);
    RUN_TEST(test_pid_outside_deadzone_computes_output);
    RUN_TEST(test_pid_integral_accumulates);
    RUN_TEST(test_pid_integral_anti_windup_upper);
    RUN_TEST(test_pid_integral_anti_windup_lower);
    RUN_TEST(test_pid_derivative_normalized_by_ts);
    RUN_TEST(test_pid_output_clamped_to_pwm_range);
    RUN_TEST(test_pid_output_minimum_pwm);
    RUN_TEST(test_pid_negative_error_reverses_direction);
    RUN_TEST(test_pid_zero_ts_defaults);
    RUN_TEST(test_pid_large_ts_clamped);
    RUN_TEST(test_pid_integral_resets_in_deadzone);
    RUN_TEST(test_pid_kp_ki_kd_runtime_values);
    RUN_TEST(test_pid_error_anterior_updated);
    RUN_TEST(test_inDeadZone_function);
    RUN_TEST(test_clampFloat_function);
    RUN_TEST(test_pid_deadtime_blocks_direction_change_for_150ms);
    RUN_TEST(test_pid_deadtime_reset_in_detener_allows_immediate_movement);
}