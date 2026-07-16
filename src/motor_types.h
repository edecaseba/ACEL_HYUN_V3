#ifndef MOTOR_TYPES_H
#define MOTOR_TYPES_H

#include <stdint.h>

enum class ActuatorDirection : uint8_t {
    STOP,
    FORWARD,
    REVERSE
};

#endif // MOTOR_TYPES_H