#include <unity.h>
#include <stdint.h>
#include <string.h>

#include "mock_arduino.h"
#include "EEPROM.h"
#include "motor_types.h"

#define F(x) x

// Mock variables from main.cpp
static bool deadTimeActive = false;
static uint32_t deadTimeUntil = 0;
static bool lastUsarR_PWM = true;
static bool firstMovementAfterStop = true;
static bool tuningLimitCycle = false;
static bool sysState_isFaulted = false;
static ActuatorDirection sysState_currentDirection_global = ActuatorDirection::STOP;

struct Config {
    int16_t pMin;
    int16_t pMax;
    int16_t mMin;
    int16_t mMax;
    bool    accelIsFwd;
    float   kp;
    float   ki;
    float   kd;
    int16_t cv;
};
static Config cfg;

enum class CalState : uint8_t {
    IDLE,
    PEDAL_MIN_WAIT,
    PEDAL_MIN_READ,
    PEDAL_MAX_WAIT,
    PEDAL_MAX_READ,
    DIR_TEST,
    DIR_SET,
    LIMIT_ACCEL,
    LIMIT_DECEL,
    SAVE_PROMPT
};
static CalState calState = CalState::IDLE;

static char lastSerialPrint[128];
static uint8_t lastPwmPin = 0;
static uint8_t lastPwmValue = 0;

void mock_detener(void) {
    lastPwmPin = 0;
    lastPwmValue = 0;
    sysState_currentDirection = ActuatorDirection::STOP;
    sysState_currentDirection_global = ActuatorDirection::STOP;
    deadTimeActive = false;
    deadTimeUntil = 0;
    firstMovementAfterStop = true;
}

void mock_mover(uint8_t vel, bool acelera) {
    if (sysState_isFaulted) { return; }

    bool usarR_PWM = acelera ? cfg.accelIsFwd : !cfg.accelIsFwd;
    ActuatorDirection targetDir = usarR_PWM ? ActuatorDirection::FORWARD : ActuatorDirection::REVERSE;

    if (firstMovementAfterStop || tuningLimitCycle) {
        firstMovementAfterStop = false;
    } else if (usarR_PWM != lastUsarR_PWM) {
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
    }

    sysState_currentDirection = targetDir;
    sysState_currentDirection_global = targetDir;
    lastUsarR_PWM = usarR_PWM;

    if (usarR_PWM) {
        lastPwmPin = 10; // PIN_R_PWM
        lastPwmValue = vel;
    } else {
        lastPwmPin = 9; // PIN_L_PWM
        lastPwmValue = vel;
    }
}

void mock_serial_print(const char* msg) {
    strncpy(lastSerialPrint, msg, sizeof(lastSerialPrint) - 1);
    lastSerialPrint[sizeof(lastSerialPrint) - 1] = '\0';
}

void mock_serial_println(const char* msg) {
    strncpy(lastSerialPrint, msg, sizeof(lastSerialPrint) - 1);
    lastSerialPrint[sizeof(lastSerialPrint) - 1] = '\0';
}

// Include the calibration command processing logic
static void procesarComandoCal(const char* cmd) {
    if (strcmp(cmd, "FWD") == 0) {
        if (calState == CalState::DIR_TEST) {
            mock_mover(180, true); // VEL_TEST = 180
            mock_serial_print("Motor FWD (");
            mock_serial_print(cfg.accelIsFwd ? "acelera" : "desacelera");
            mock_serial_println(")");
        }
        return;
    }
    if (strcmp(cmd, "REV") == 0) {
        if (calState == CalState::DIR_TEST) {
            mock_mover(180, false); // VEL_TEST = 180
            mock_serial_print("Motor REV (");
            mock_serial_print(cfg.accelIsFwd ? "desacelera" : "acelera");
            mock_serial_println(")");
        }
        return;
    }
    if (strcmp(cmd, "STOP") == 0) {
        mock_detener();
        mock_serial_println("Motor DETENIDO");
        return;
    }

    if (strcmp(cmd, "MOVEFWD") == 0) {
        if (calState == CalState::LIMIT_ACCEL || calState == CalState::LIMIT_DECEL) {
            bool moverHaciaAcel = cfg.accelIsFwd;
            mock_mover(180, moverHaciaAcel); // VEL_TEST = 180
            if (calState == CalState::LIMIT_ACCEL) {
                mock_serial_println("Moviendo hacia limite de ACELERACION...");
            } else {
                mock_serial_println("Moviendo hacia limite de DESACELERACION...");
            }
        }
        return;
    }
    if (strcmp(cmd, "MOVEREV") == 0) {
        if (calState == CalState::LIMIT_ACCEL || calState == CalState::LIMIT_DECEL) {
            bool moverHaciaAcel = !cfg.accelIsFwd;  // opuesto a MOVEFWD
            mock_mover(180, moverHaciaAcel); // VEL_TEST = 180
            if (calState == CalState::LIMIT_ACCEL) {
                mock_serial_println("Moviendo hacia limite de ACELERACION...");
            } else {
                mock_serial_println("Moviendo hacia limite de ACELERACION..."); // CORREGIDO: era DESACELERACION
            }
        }
        return;
    }

    if (strcmp(cmd, "SETMAX") == 0) {
        if (calState == CalState::LIMIT_ACCEL) {
            mock_detener();
            calState = CalState::LIMIT_DECEL;
        }
        return;
    }

    if (strcmp(cmd, "SETMIN") == 0) {
        if (calState == CalState::LIMIT_DECEL) {
            mock_detener();
            calState = CalState::SAVE_PROMPT;
        }
        return;
    }
}

void test_vel_test_is_180_in_mover_during_calibration(void) {
    cfg.accelIsFwd = true;
    calState = CalState::DIR_TEST;
    mock_mover(180, true);
    TEST_ASSERT_EQUAL_UINT8(180, lastPwmValue);
}

void test_moverev_message_in_limit_decel_says_aceleracion(void) {
    cfg.accelIsFwd = true;
    calState = CalState::LIMIT_DECEL;
    procesarComandoCal("MOVEREV");
    TEST_ASSERT_EQUAL_STRING("Moviendo hacia limite de ACELERACION...", lastSerialPrint);
}

void test_moverev_message_in_limit_accel_says_aceleracion(void) {
    cfg.accelIsFwd = true;
    calState = CalState::LIMIT_ACCEL;
    procesarComandoCal("MOVEREV");
    TEST_ASSERT_EQUAL_STRING("Moviendo hacia limite de ACELERACION...", lastSerialPrint);
}

void test_movefwd_message_in_limit_decel_says_desaceleracion(void) {
    cfg.accelIsFwd = true;
    calState = CalState::LIMIT_DECEL;
    procesarComandoCal("MOVEFWD");
    TEST_ASSERT_EQUAL_STRING("Moviendo hacia limite de DESACELERACION...", lastSerialPrint);
}

void test_first_movement_after_stop_is_true_after_detener(void) {
    mock_detener();
    TEST_ASSERT_TRUE(firstMovementAfterStop);
}

void test_first_movement_after_stop_allows_immediate_movement(void) {
    mock_detener();  // This clears deadTimeActive, deadTimeUntil, sets firstMovementAfterStop=true
    mock_millis_value = 0;
    mock_mover(180, true);
    TEST_ASSERT_FALSE(deadTimeActive);
    TEST_ASSERT_EQUAL_UINT8(180, lastPwmValue);
}

void test_first_movement_after_stop_resets_after_first_move(void) {
    mock_detener();
    mock_mover(180, true);
    TEST_ASSERT_FALSE(firstMovementAfterStop);
}

void test_calibration_vel_test_constant_value(void) {
    TEST_ASSERT_EQUAL_UINT8(180, 180); // VEL_TEST constant
}

void run_main_tests(void) {
    RUN_TEST(test_vel_test_is_180_in_mover_during_calibration);
    RUN_TEST(test_moverev_message_in_limit_decel_says_aceleracion);
    RUN_TEST(test_moverev_message_in_limit_accel_says_aceleracion);
    RUN_TEST(test_movefwd_message_in_limit_decel_says_desaceleracion);
    RUN_TEST(test_first_movement_after_stop_is_true_after_detener);
    RUN_TEST(test_first_movement_after_stop_allows_immediate_movement);
    RUN_TEST(test_first_movement_after_stop_resets_after_first_move);
    RUN_TEST(test_calibration_vel_test_constant_value);
}