#include <Arduino.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <string.h>

#include "pid_controller.h"
#include <EEPROM.h>
#include "overcurrent.h"

template<typename T>
bool eepromPutSafe(int addr, const T& value) {
    if (addr < 0 || addr + sizeof(T) > EEPROM.length()) return false;
    EEPROM.put(addr, value);
    return true;
}

template<typename T>
bool eepromGetSafe(int addr, T& value) {
    if (addr < 0 || addr + sizeof(T) > EEPROM.length()) return false;
    EEPROM.get(addr, value);
    return true;
}

static void ucase(char* s) {
    while (*s) {
        if (*s >= 'a' && *s <= 'z') *s = static_cast<char>(*s - ('a' - 'A'));
        ++s;
    }
}

constexpr uint8_t pinPotOp     = A0;
constexpr uint8_t pinPotFeed   = A1;
constexpr uint8_t PIN_IS_SENSE = A2;

constexpr uint8_t PIN_EN       = 8;
constexpr uint8_t PIN_R_PWM    = 10;
constexpr uint8_t PIN_L_PWM    = 9;

constexpr uint16_t DEAD_TIME_MS   = 150;
constexpr uint16_t STALL_CURRENT_ADC = 950;

constexpr uint8_t VEL_TEST       = 140;
constexpr uint8_t TUNE_PWM       = 180;

constexpr uint32_t TUNE_TIMEOUT_MS       = 90000;
constexpr uint32_t TUNE_INIT_TIMEOUT_MS  = 10000;
constexpr int16_t  TUNE_RELAY_HYSTERESIS = 3;
constexpr uint8_t  TUNE_MIN_CYCLES       = 4;
constexpr uint32_t TUNE_PROGRESS_INTERVAL_MS = 5000;

constexpr uint16_t TIEMPO_ASENTAMIENTO_MS = 100;
constexpr uint8_t  UMBRAL_REACTIVACION    = 6;

static void configurarPinesSinUso()
{
    const uint8_t pinesPullUp[] = {2,3,4,5,6,7,11,12,13};
    for (uint8_t i = 0; i < sizeof(pinesPullUp); ++i) {
        pinMode(pinesPullUp[i], INPUT_PULLUP);
    }
    digitalWrite(PIN_L_PWM, LOW);
    digitalWrite(PIN_R_PWM, LOW);
    digitalWrite(PIN_EN, LOW);
}

constexpr int16_t  MAGIC_NUMBER   = 111;
constexpr float    DEFAULT_KP     = 2.0f;
constexpr float    DEFAULT_KI     = 0.1f;
constexpr float    DEFAULT_KD     = 0.5f;

struct EMAFilter {
    float alpha;
    float filteredValue;

    void init(float initialValue) {
        filteredValue = initialValue;
    }

    void update(float newValue) {
        filteredValue = (alpha * newValue) + (1.0f - alpha) * filteredValue;
    }
};

constexpr float ALPHA_PEDAL    = 0.75f;
constexpr float ALPHA_FEEDBACK = 0.80f;
constexpr float ALPHA_CURRENT  = 0.85f;

static EMAFilter filterPedal    = { ALPHA_PEDAL, 0.0f };
static EMAFilter filterFeedback = { ALPHA_FEEDBACK, 0.0f };
static EMAFilter filterCurrent  = { ALPHA_CURRENT, 0.0f };

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

enum class SystemMode : uint8_t {
    OPERATION,
    CALIBRATION,
    TUNING
};
static SystemMode sysMode = SystemMode::OPERATION;

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

enum class TunePhase : uint8_t {
    IDLE,
    INIT_MOVE,
    LIMIT_CYCLE,      // Bang-bang entre mMin y mMax
    CYCLES_DONE,
    CALCULATE,
    SAVE_PENDING,     // PID calculado pero no validado (amplitud baja)
    SAVE
};

struct TuneContext {
    TunePhase phase;
    int16_t   targetPos;
    uint32_t  phaseStartMs;
    uint32_t  lastSwitchUs;
    uint32_t  firstSwitchUs;
    float     periods[8];
    float     peaks[8];
    float     valleys[8];
    uint8_t   cycleCount;
    float     currentPeak;
    float     currentValley;
    bool      movingToMax;
    float     Tu;
    float     Ku;
    uint32_t  lastProgressMs;
    int16_t   lastPos;          // Para detectar inversión de dirección
    uint32_t  lastImprovementMs; // Tiempo sin mejorar posición
};
static TuneContext tuneCtx;

#include "motor_types.h"

struct MotorState {
    ActuatorDirection currentDirection;
    bool isFaulted;
};
static MotorState sysState = { ActuatorDirection::STOP, false };

ActuatorDirection sysState_currentDirection = ActuatorDirection::STOP;

static uint32_t deadTimeUntil  = 0;
static bool     deadTimeActive = false;
static bool     lastUsarR_PWM  = true;
static bool     firstMovementAfterStop = true;
static bool     tuningLimitCycle = false;  // Desactiva dead-time en LIMIT_CYCLE

static bool     asentado           = false;
static uint32_t asentadoTimer      = 0;
static int16_t  errorAnterior      = 0;
static float    integralAccumulator = 0.0f;
static uint32_t lastPidTime        = 0;

static uint32_t lastStallCheck = 0;
constexpr uint16_t STALL_CHECK_INTERVAL_MS = 50;

static char cmdBuffer[24];
static uint8_t cmdIndex = 0;

static void detener();
static void mover(uint8_t vel, bool acelera);
static void iniciarCalibracion();
static void tickCalibration();
static void iniciarTuning();
static void tickTuning();
static void tickOperation();
static void procesarComando();
static void procesarComandoCal();
static void comandoOK();
static void comandoSAVE();
static void comandoDIR(const char* arg);
static void registrarCruce(int16_t posicion);

static void detener() {
    digitalWrite(PIN_R_PWM, LOW);
    digitalWrite(PIN_L_PWM, LOW);
    sysState.currentDirection = ActuatorDirection::STOP;
    sysState_currentDirection = ActuatorDirection::STOP;
    deadTimeActive = false;
    deadTimeUntil = 0;
    firstMovementAfterStop = true;
}

#ifdef UNITY_TEST
bool test_getDeadTimeActive(void) { return deadTimeActive; }
uint32_t test_getDeadTimeUntil(void) { return deadTimeUntil; }
void test_setDeadTimeActive(bool v) { deadTimeActive = v; }
void test_setDeadTimeUntil(uint32_t v) { deadTimeUntil = v; }
void test_setMockMillis(uint32_t v) { mock_millis_value = v; }
#endif

void safeState() {
    detener();
    digitalWrite(PIN_EN, LOW);
    sysState.isFaulted = true;
}

void initMotorHardware() {
    digitalWrite(PIN_EN, LOW);
    digitalWrite(PIN_R_PWM, LOW);
    digitalWrite(PIN_L_PWM, LOW);

    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_R_PWM, OUTPUT);
    pinMode(PIN_L_PWM, OUTPUT);
    pinMode(PIN_IS_SENSE, INPUT);

    // Fast PWM 10-bit, TOP=ICR1=799, prescaler=1 -> 16MHz/(800*1) = 20 kHz
    // WGM13:10 = 0b1111 (Fast PWM, TOP=ICR1)
    TCCR1A = (TCCR1A & 0xF0) | static_cast<uint8_t>((1 << WGM11) | (1 << WGM10));
    TCCR1B = (TCCR1B & 0xF0) | static_cast<uint8_t>((1 << WGM13) | (1 << WGM12) | (1 << CS10));
    ICR1 = 799;

    digitalWrite(PIN_EN, HIGH);
}

void mover(uint8_t vel, bool acelera) {
    if (sysState.isFaulted) { return; }

    bool usarR_PWM = acelera ? cfg.accelIsFwd : !cfg.accelIsFwd;
    ActuatorDirection targetDir = usarR_PWM ? ActuatorDirection::FORWARD : ActuatorDirection::REVERSE;

    // Saltar dead-time en primer movimiento tras detenerse (ambos PWM en LOW = seguro)
    // También saltar dead-time durante LIMIT_CYCLE tuning (cambios rápidos controlados)
    if (firstMovementAfterStop || tuningLimitCycle) {
        firstMovementAfterStop = false;
    } else if (usarR_PWM != lastUsarR_PWM) {
        if (!deadTimeActive) {
            detener();
            deadTimeUntil = millis() + DEAD_TIME_MS;
            deadTimeActive = true;
        }
        return;
    }

    if (deadTimeActive) {
        if (millis() < deadTimeUntil) { return; }
        deadTimeActive = false;
    }

    sysState.currentDirection = targetDir;
    sysState_currentDirection = targetDir;
    lastUsarR_PWM = usarR_PWM;

    if (usarR_PWM) {
        analogWrite(PIN_L_PWM, 0);
        analogWrite(PIN_R_PWM, vel);
    } else {
        analogWrite(PIN_R_PWM, 0);
        analogWrite(PIN_L_PWM, vel);
    }
}

void monitorStallCurrent() {
    if (sysState.currentDirection == ActuatorDirection::STOP) { return; }

    uint32_t now = millis();
    if (now - lastStallCheck < STALL_CHECK_INTERVAL_MS) { return; }
    lastStallCheck = now;

    uint16_t rawAdc = static_cast<uint16_t>(analogRead(PIN_IS_SENSE));
    filterCurrent.update(static_cast<float>(rawAdc));

    if (static_cast<uint16_t>(filterCurrent.filteredValue) >= STALL_CURRENT_ADC) {
        detener();
        digitalWrite(PIN_EN, LOW);
        sysState.isFaulted = true;
        integralAccumulator = 0.0f;
        errorAnterior = 0;
        Serial.println(F("CRITICAL: STALL DETECTADO. Puente H bloqueado."));
    }
}

static void iniciarCalibracion() {
    sysMode = SystemMode::CALIBRATION;
    sysState.isFaulted = false;
    integralAccumulator = 0.0f;
    errorAnterior = 0;
    asentado = false;
    digitalWrite(PIN_EN, HIGH);
    detener();

    calState = CalState::PEDAL_MIN_WAIT;
    Serial.println(F("\n=== CALIBRACION INTERACTIVA ==="));
    Serial.println(F("Paso 1/6: Coloque el acelerador en RALENTI (minimo) y envie OK"));
}

static void tickCalibration() {
    switch (calState) {
        case CalState::IDLE:
        case CalState::PEDAL_MIN_WAIT:
        case CalState::PEDAL_MAX_WAIT:
        case CalState::DIR_TEST:
        case CalState::DIR_SET:
        case CalState::LIMIT_ACCEL:
        case CalState::LIMIT_DECEL:
        case CalState::SAVE_PROMPT:
            break;

        case CalState::PEDAL_MIN_READ: {
            cfg.pMin = static_cast<int16_t>(filterPedal.filteredValue);
            Serial.print(F("pMin = ")); Serial.print(cfg.pMin);
            Serial.println(F(" guardado."));
            calState = CalState::PEDAL_MAX_WAIT;
            Serial.println(F("Paso 2/6: Coloque el acelerador a MAXIMAS RPM y envie OK"));
            break;
        }

        case CalState::PEDAL_MAX_READ: {
            cfg.pMax = static_cast<int16_t>(filterPedal.filteredValue);
            Serial.print(F("pMax = ")); Serial.print(cfg.pMax);
            Serial.println(F(" guardado."));
            calState = CalState::DIR_TEST;
            Serial.println(F("Paso 3/6: Use FWD/REV/STOP para probar direccion del motor."));
            Serial.println(F("Luego envie: DIR FWD ACEL  o  DIR REV ACEL"));
            break;
        }
    }
}

static void comandoOK() {
    if (sysMode != SystemMode::CALIBRATION) { return; }

    switch (calState) {
        case CalState::PEDAL_MIN_WAIT:
            calState = CalState::PEDAL_MIN_READ;
            break;
        case CalState::PEDAL_MAX_WAIT:
            calState = CalState::PEDAL_MAX_READ;
            break;
        default:
            Serial.println(F("OK no valido en este paso."));
            break;
    }
}

static void comandoSAVE() {
    if (sysMode != SystemMode::CALIBRATION) { return; }
    if (calState != CalState::SAVE_PROMPT) {
        Serial.println(F("SAVE no valido aqui. Complete todos los pasos."));
        return;
    }

    if (abs(static_cast<int>(cfg.mMax - cfg.mMin)) < 50) {
        Serial.println(F("ERROR: Recorrido del actuador muy pequeno (<50). Recalibre."));
        return;
    }
    if (abs(static_cast<int>(cfg.pMax - cfg.pMin)) < 50) {
        Serial.println(F("ERROR: Recorrido del pedal muy pequeno (<50). Recalibre."));
        return;
    }

    cfg.cv = MAGIC_NUMBER;
    if (!eepromPutSafe(0, cfg)) { Serial.println(F("EEPROM write failed!")); }
    Serial.println(F("=== CALIBRACION GUARDADA EN EEPROM ==="));
    Serial.println(F("Envie TUNE para auto-ajustar PID, o cambie a operacion normal."));
    sysMode = SystemMode::OPERATION;
    calState = CalState::IDLE;
}

static void comandoDIR(const char* arg) {
    if (sysMode != SystemMode::CALIBRATION) { return; }
    if (calState != CalState::DIR_TEST && calState != CalState::DIR_SET) {
        Serial.println(F("DIR no valido aqui. Termine el test de direccion primero."));
        return;
    }

    if (strstr(arg, "FWD ACEL") != nullptr || strcmp(arg, "FWD") == 0) {
        cfg.accelIsFwd = true;
        calState = CalState::LIMIT_ACCEL;
        Serial.println(F("Direccion: R_PWM ACELERA. Guardado."));
        Serial.println(F("Paso 4/6: Use MOVEFWD para ir al tope de ACELERACION."));
        Serial.println(F("Cuando llegue, envie SETMAX"));
    } else if (strstr(arg, "REV ACEL") != nullptr || strcmp(arg, "REV") == 0) {
        cfg.accelIsFwd = false;
        calState = CalState::LIMIT_ACCEL;
        Serial.println(F("Direccion: L_PWM ACELERA. Guardado."));
        Serial.println(F("Paso 4/6: Use MOVEREV para ir al tope de ACELERACION."));
        Serial.println(F("Cuando llegue, envie SETMAX"));
    } else {
        Serial.println(F("Use: DIR FWD ACEL  o  DIR REV ACEL"));
    }
}

static void procesarComandoCal() {
    if (strcmp(cmdBuffer, "FWD") == 0) {
        if (calState == CalState::DIR_TEST) {
            mover(VEL_TEST, true);
            Serial.print(F("Motor FWD ("));
            Serial.print(cfg.accelIsFwd ? F("acelera") : F("desacelera"));
            Serial.println(F(")"));
        }
        return;
    }
    if (strcmp(cmdBuffer, "REV") == 0) {
        if (calState == CalState::DIR_TEST) {
            mover(VEL_TEST, false);
            Serial.print(F("Motor REV ("));
            Serial.print(cfg.accelIsFwd ? F("desacelera") : F("acelera"));
            Serial.println(F(")"));
        }
        return;
    }
    if (strcmp(cmdBuffer, "STOP") == 0) {
        detener();
        Serial.println(F("Motor DETENIDO"));
        return;
    }

    if (strcmp(cmdBuffer, "MOVEFWD") == 0) {
        if (calState == CalState::LIMIT_ACCEL || calState == CalState::LIMIT_DECEL) {
            bool moverHaciaAcel = cfg.accelIsFwd;  // true = FWD acelera, false = REV acelera
            mover(VEL_TEST, moverHaciaAcel);
            if (calState == CalState::LIMIT_ACCEL) {
                Serial.println(F("Moviendo hacia limite de ACELERACION..."));
            } else {
                Serial.println(F("Moviendo hacia limite de DESACELERACION..."));
            }
        }
        return;
    }
    if (strcmp(cmdBuffer, "MOVEREV") == 0) {
        if (calState == CalState::LIMIT_ACCEL || calState == CalState::LIMIT_DECEL) {
            bool moverHaciaAcel = !cfg.accelIsFwd;  // opuesto a MOVEFWD
            mover(VEL_TEST, moverHaciaAcel);
            if (calState == CalState::LIMIT_ACCEL) {
                Serial.println(F("Moviendo hacia limite de ACELERACION..."));
            } else {
                Serial.println(F("Moviendo hacia limite de DESACELERACION..."));
            }
        }
        return;
    }

    if (strcmp(cmdBuffer, "SETMAX") == 0) {
        if (calState == CalState::LIMIT_ACCEL) {
            cfg.mMax = static_cast<int16_t>(filterFeedback.filteredValue);
            detener();
            Serial.print(F("mMax = ")); Serial.print(cfg.mMax);
            Serial.println(F(" guardado (tope ACELERACION)."));
            calState = CalState::LIMIT_DECEL;
            if (cfg.accelIsFwd) {
                Serial.println(F("Paso 5/6: Use MOVEREV para ir al tope de DESACELERACION."));
            } else {
                Serial.println(F("Paso 5/6: Use MOVEFWD para ir al tope de DESACELERACION."));
            }
            Serial.println(F("Cuando llegue, envie SETMIN"));
        } else {
            Serial.println(F("SETMAX no valido aqui. Use en paso 4 (tope ACELERACION)."));
        }
        return;
    }

    if (strcmp(cmdBuffer, "SETMIN") == 0) {
        if (calState == CalState::LIMIT_DECEL) {
            cfg.mMin = static_cast<int16_t>(filterFeedback.filteredValue);
            detener();
            Serial.print(F("mMin = ")); Serial.print(cfg.mMin);
            Serial.println(F(" guardado (tope DESACELERACION)."));
            calState = CalState::SAVE_PROMPT;
            Serial.println(F("Paso 6/6: Envie SAVE para guardar en EEPROM"));
            Serial.println(F("         o TUNE para auto-ajustar PID primero."));
        } else {
            Serial.println(F("SETMIN no valido aqui. Use en paso 5 (tope DESACELERACION)."));
        }
        return;
    }

    if (strcmp(cmdBuffer, "OK") == 0) {
        comandoOK();
        return;
    }

    if (strncmp(cmdBuffer, "DIR ", 4) == 0) {
        comandoDIR(cmdBuffer + 4);
        return;
    }

    if (strcmp(cmdBuffer, "SAVE") == 0) {
        comandoSAVE();
        return;
    }

    Serial.print(F("Comando no reconocido: "));
    Serial.println(cmdBuffer);
}

static void iniciarTuning() {
    if (sysMode == SystemMode::CALIBRATION) {
        cfg.cv = 0;
    }

    sysMode = SystemMode::TUNING;
    sysState.isFaulted = false;
    integralAccumulator = 0.0f;
    errorAnterior = 0;
    asentado = false;
    detener();

    tuneCtx.phase = TunePhase::INIT_MOVE;
    tuneCtx.phaseStartMs = millis();
    tuneCtx.cycleCount = 0;
    tuneCtx.currentPeak = -1000.0f;
    tuneCtx.currentValley = 1000.0f;
    tuneCtx.Tu = 0.0f;
    tuneCtx.Ku = 0.0f;
    tuneCtx.firstSwitchUs = 0;
    tuneCtx.lastProgressMs = 0;
    tuneCtx.movingToMax = true;  // Empezar hacia mMax

    Serial.println(F("\n=== AUTO-TUNING PID (LIMIT CYCLE) ==="));
    Serial.print(F("Rango: mMin=")); Serial.print(cfg.mMin);
    Serial.print(F(" mMax=")); Serial.println(cfg.mMax);
    Serial.println(F("Moviendo a tope ACELERACION (mMax)..."));
}

static void tickTuning() {
    uint32_t nowMs = millis();

    int16_t rawPos = static_cast<int16_t>(filterFeedback.filteredValue);
    int16_t posicion = map(rawPos, cfg.mMin, cfg.mMax, 0, 100);
    posicion = constrain(posicion, 0, 100);

    // Progreso cada 5s
    if (tuneCtx.phase == TunePhase::LIMIT_CYCLE && nowMs - tuneCtx.lastProgressMs >= TUNE_PROGRESS_INTERVAL_MS) {
        tuneCtx.lastProgressMs = nowMs;
        Serial.print(F("TUNE: pos=")); Serial.print(posicion);
        Serial.print(F(" dir=")); Serial.print(tuneCtx.movingToMax ? F("->mMax") : F("->mMin"));
        Serial.print(F(" ciclos=")); Serial.println(tuneCtx.cycleCount);
    }

    switch (tuneCtx.phase) {
        case TunePhase::IDLE:
            break;

        case TunePhase::INIT_MOVE:
            // Mover hacia mMax (tope ACELERACION) primero
            if (posicion < 95) {
                mover(TUNE_PWM, true);  // Hacia mMax
            } else {
                detener();
                tuneCtx.phase = TunePhase::LIMIT_CYCLE;
                tuneCtx.phaseStartMs = nowMs;
                tuneCtx.movingToMax = false;  // Ahora hacia mMin
                tuneCtx.lastSwitchUs = micros();
                tuneCtx.currentPeak = -1000.0f;
                tuneCtx.currentValley = 1000.0f;
                tuneCtx.lastProgressMs = nowMs;
                tuneCtx.lastPos = posicion;
                tuneCtx.lastImprovementMs = nowMs;
                // Reset dead-time y fault para permitir oscilación libre
                deadTimeActive = false;
                deadTimeUntil = 0;
                firstMovementAfterStop = true;
                tuningLimitCycle = true;  // Desactiva dead-time en LIMIT_CYCLE
                sysState.isFaulted = false;
                digitalWrite(PIN_EN, HIGH);
                Serial.println(F("TUNE: En mMax. Iniciando ciclo limite mMax<->mMin..."));
            }

            if (nowMs - tuneCtx.phaseStartMs > TUNE_INIT_TIMEOUT_MS) {
                detener();
                Serial.println(F("TUNE: Timeout alcanzando mMax. Abortando."));
                tuneCtx.phase = TunePhase::IDLE;
                sysMode = SystemMode::OPERATION;
            }
            break;

        case TunePhase::LIMIT_CYCLE: {
            int16_t rawPos = static_cast<int16_t>(filterFeedback.filteredValue);
            int16_t posicion = map(rawPos, cfg.mMin, cfg.mMax, 0, 100);
            posicion = constrain(posicion, 0, 100);

            // Progreso cada 5s
            if (nowMs - tuneCtx.lastProgressMs >= TUNE_PROGRESS_INTERVAL_MS) {
                tuneCtx.lastProgressMs = nowMs;
                Serial.print(F("TUNE: pos=")); Serial.print(posicion);
                Serial.print(F(" dir=")); Serial.print(tuneCtx.movingToMax ? F("->mMax") : F("->mMin"));
                Serial.print(F(" ciclos=")); Serial.println(tuneCtx.cycleCount);
            }

            // Detectar inversión de dirección: si la posición deja de mejorar en la dirección actual
            bool mejorando = false;
            if (tuneCtx.movingToMax) {
                if (posicion > tuneCtx.lastPos) mejorando = true;
            } else {
                if (posicion < tuneCtx.lastPos) mejorando = true;
            }

            if (mejorando) {
                tuneCtx.lastPos = posicion;
                tuneCtx.lastImprovementMs = nowMs;
            } else {
                // No mejora: si pasaron 2s sin mejorar, cambiar dirección
                if (nowMs - tuneCtx.lastImprovementMs > 2000) {
                    registrarCruce(posicion);
                    tuneCtx.movingToMax = !tuneCtx.movingToMax;
                    tuneCtx.lastImprovementMs = nowMs;
                }
            }

            // Mover hacia el tope actual
            if (tuneCtx.movingToMax) {
                mover(TUNE_PWM, true);
            } else {
                mover(TUNE_PWM, false);
            }

            if (nowMs - tuneCtx.phaseStartMs > TUNE_TIMEOUT_MS) {
                detener();
                Serial.println(F("TUNE: Timeout 90s. Abortando."));
                tuneCtx.phase = TunePhase::IDLE;
                sysMode = SystemMode::OPERATION;
                sysState.isFaulted = false;
                integralAccumulator = 0.0f;
                errorAnterior = 0;
                asentado = false;
                tuningLimitCycle = false;
                digitalWrite(PIN_EN, HIGH);
            }
            break;
        }

        case TunePhase::CYCLES_DONE:
            detener();
            tuningLimitCycle = false;  // Reactiva dead-time
            tuneCtx.phase = TunePhase::CALCULATE;
            break;

        case TunePhase::CALCULATE: {
            if (tuneCtx.cycleCount < TUNE_MIN_CYCLES) {
                Serial.println(F("TUNE: Ciclos insuficientes. Abortando."));
                tuneCtx.phase = TunePhase::IDLE;
                sysMode = SystemMode::OPERATION;
                break;
            }

            // Usar los picos/valles REALES medidos durante la oscilación completa
            // Encontrar el máximo global y mínimo global de todos los ciclos
            float globalPeak = -1000.0f;
            float globalValley = 1000.0f;
            for (uint8_t i = 0; i < tuneCtx.cycleCount; ++i) {
                if (tuneCtx.peaks[i] > globalPeak) globalPeak = tuneCtx.peaks[i];
                if (tuneCtx.valleys[i] < globalValley) globalValley = tuneCtx.valleys[i];
            }

            float amplitudGlobal = (globalPeak - globalValley) / 2.0f;

            // Calcular período promedio usando cruces válidos (período completo = 2 * media ciclo)
            float sumPeriod = 0.0f;
            uint8_t validPeriods = 0;
            for (uint8_t i = 0; i < tuneCtx.cycleCount; ++i) {
                if (tuneCtx.periods[i] > 0.0f && tuneCtx.periods[i] < 100.0f) {  // Período razonable < 100s
                    sumPeriod += tuneCtx.periods[i];
                    ++validPeriods;
                }
            }

            // Validación MUY permisiva: si oscilamos entre límites reales, aceptamos
            if (validPeriods == 0 || amplitudGlobal < 5.0f) {  // Amplitud mínima 5% del rango
                Serial.println(F("TUNE: Oscilacion insuficiente. PID calculado pero no validado."));
                Serial.println(F("Envia SAVEPID para guardar anyway, o CAL para recalibrar."));
                tuneCtx.phase = TunePhase::SAVE_PENDING;
                break;
            }

            float Tu = sumPeriod / static_cast<float>(validPeriods);
            float a  = amplitudGlobal;

            constexpr float H_ESTIMADO = 50.0f;
            constexpr float PI_F = 3.14159265f;
            float Ku = 4.0f * H_ESTIMADO / (PI_F * a);

            float Kp = 0.6f * Ku;
            float Ki = 1.2f * Ku / Tu;
            float Kd = 0.075f * Ku * Tu;

            Kp = clampFloat(Kp, 0.1f, 50.0f);
            Ki = clampFloat(Ki, 0.0f, 20.0f);
            Kd = clampFloat(Kd, 0.0f, 20.0f);

            cfg.kp = Kp;
            cfg.ki = Ki;
            cfg.kd = Kd;

            tuneCtx.Tu = Tu;
            tuneCtx.Ku = Ku;

            Serial.println(F("\n=== AUTO-TUNING COMPLETO ==="));
            Serial.print(F("Rango real: ")); Serial.print(globalValley, 1); Serial.print(F(" - ")); Serial.println(globalPeak, 1);
            Serial.print(F("Amplitud: ")); Serial.print(a, 1); Serial.println(F("%"));
            Serial.print(F("Tu = ")); Serial.print(Tu, 3); Serial.println(F(" s"));
            Serial.print(F("Ku = ")); Serial.println(Ku, 2);
            Serial.print(F("Kp = ")); Serial.println(Kp, 2);
            Serial.print(F("Ki = ")); Serial.println(Ki, 4);
            Serial.print(F("Kd = ")); Serial.println(Kd, 4);

            tuneCtx.phase = TunePhase::SAVE;
            break;
        }

        case TunePhase::SAVE_PENDING: {
            // PID calculado pero no validado - espera SAVEPID o CAL
            break;
        }

        case TunePhase::SAVE: {
            cfg.cv = MAGIC_NUMBER;
            if (!eepromPutSafe(0, cfg)) { Serial.println(F("EEPROM write failed!")); }
            Serial.println(F("Valores guardados en EEPROM."));
            Serial.println(F("Envie CAL para recalibrar limites, o RST para operacion normal."));

            tuneCtx.phase = TunePhase::IDLE;
            sysMode = SystemMode::OPERATION;
            calState = CalState::IDLE;
            break;
        }
    }
}

static void registrarCruce(int16_t posicion) {
    uint32_t nowUs = micros();

    if (tuneCtx.firstSwitchUs == 0) {
        tuneCtx.firstSwitchUs = nowUs;
        tuneCtx.lastSwitchUs = nowUs;
        tuneCtx.currentPeak = -1000.0f;
        tuneCtx.currentValley = 1000.0f;
        return;
    }

    float dt = static_cast<float>(static_cast<int32_t>(nowUs - tuneCtx.lastSwitchUs)) / 1000000.0f;
    tuneCtx.lastSwitchUs = nowUs;

    if (dt > 0.005f && dt < 10.0f) {
        if (tuneCtx.cycleCount < 8) {
            tuneCtx.periods[tuneCtx.cycleCount] = dt * 2.0f;
            tuneCtx.peaks[tuneCtx.cycleCount] = tuneCtx.currentPeak;
            tuneCtx.valleys[tuneCtx.cycleCount] = tuneCtx.currentValley;
        }
        ++tuneCtx.cycleCount;

        Serial.print(F("."));
    }

    tuneCtx.currentPeak = -1000.0f;
    tuneCtx.currentValley = 1000.0f;

    if (tuneCtx.cycleCount >= TUNE_MIN_CYCLES) {
        tuneCtx.phase = TunePhase::CYCLES_DONE;
        Serial.println(F("\nTUNE: Ciclos suficientes."));
    }
}

static void tickOperation() {
    if (sysState.isFaulted) { return; }
    if (cfg.cv != MAGIC_NUMBER) { return; }

    uint32_t nowMs = millis();

    int16_t rOp = static_cast<int16_t>(filterPedal.filteredValue);
    int16_t rFe = static_cast<int16_t>(filterFeedback.filteredValue);

    int16_t setpoint = map(rOp, cfg.pMin, cfg.pMax, 0, 100);
    int16_t posicion = map(rFe, cfg.mMin, cfg.mMax, 0, 100);

    setpoint = constrain(setpoint, 0, 100);
    posicion = constrain(posicion, 0, 100);

    int16_t error = static_cast<int16_t>(setpoint - posicion);

    if (asentado) {
        if (abs(static_cast<int>(error)) > static_cast<int16_t>(UMBRAL_REACTIVACION)) {
            asentado = false;
            lastPidTime = micros();
        }
    }

    if (!asentado) {
        if (inDeadZone(error)) {
            if (asentadoTimer == 0) {
                asentadoTimer = nowMs;
            } else if (nowMs - asentadoTimer >= TIEMPO_ASENTAMIENTO_MS) {
                detener();
                integralAccumulator = 0.0f;
                errorAnterior = 0;
                asentado = true;
                asentadoTimer = 0;
                return;
            }
        } else {
            asentadoTimer = 0;
        }

        uint32_t nowUs = micros();
        float Ts = static_cast<float>(static_cast<int32_t>(nowUs - lastPidTime)) / 1000000.0f;
        if (Ts <= 0.0f || Ts > 0.1f) { Ts = 0.01f; }
        lastPidTime = nowUs;

        bool direccionHaciaMax = (error > 0);
        bool enLimiteMecanico = false;

        if (!inDeadZone(error)) {
            if (direccionHaciaMax && (rFe >= cfg.mMax)) {
                detener();
                integralAccumulator = 0.0f;
                errorAnterior = 0;
                enLimiteMecanico = true;
            } else if (!direccionHaciaMax && (rFe <= cfg.mMin)) {
                detener();
                integralAccumulator = 0.0f;
                errorAnterior = 0;
                enLimiteMecanico = true;
            }
        }

        if (!enLimiteMecanico) {
            PidInput pidIn;
            pidIn.errorActual = error;
            pidIn.errorAnterior = errorAnterior;
            pidIn.integralAccumulator = integralAccumulator;
            pidIn.Ts = Ts;
            pidIn.kp = cfg.kp;
            pidIn.ki = cfg.ki;
            pidIn.kd = cfg.kd;

            PidOutput pidOut;
            pidCompute(pidIn, pidOut);

            integralAccumulator = pidOut.integralAccumulator;
            errorAnterior = pidOut.errorAnterior;

            if (pidOut.deberiaDetener) {
                detener();
            } else {
                mover(pidOut.vel, direccionHaciaMax);
            }
        }
    }

    static uint32_t reportTimer = 0;
    if (nowMs - reportTimer > 250) {
        Serial.print(F("SetP:")); Serial.print(setpoint);
        Serial.print(F(" Act:")); Serial.print(posicion);
        Serial.print(F(" Err:")); Serial.print(error);
        if (asentado) { Serial.print(F(" [ASENTADO]")); }
        Serial.print(F(" Kp:")); Serial.print(cfg.kp, 1);
        Serial.print(F(" Ki:")); Serial.print(cfg.ki, 2);
        Serial.print(F(" Kd:")); Serial.println(cfg.kd, 2);
        reportTimer = nowMs;
    }
}

static void resetFault() {
    sysState.isFaulted = false;
    integralAccumulator = 0.0f;
    errorAnterior = 0;
    asentado = false;
    digitalWrite(PIN_EN, HIGH);
    Serial.println(F("Fallo reseteado. Sistema listo."));
}

static void procesarComando() {
    if (strcmp(cmdBuffer, "CAL") == 0) {
        iniciarCalibracion();
        return;
    }
    if (strcmp(cmdBuffer, "RST") == 0) {
        resetFault();
        return;
    }
    if (strcmp(cmdBuffer, "OCAL") == 0) {
        extern void oc_calibrate(void);
        oc_calibrate();
        Serial.println(F("[OC] Recalibracion iniciada..."));
        return;
    }
    if (strcmp(cmdBuffer, "TUNE") == 0) {
        if (cfg.cv != MAGIC_NUMBER && calState != CalState::SAVE_PROMPT) {
            Serial.println(F("Complete la calibracion (CAL) antes de tunear."));
            return;
        }
        iniciarTuning();
        return;
    }
    if (strcmp(cmdBuffer, "STOP") == 0) {
        detener();
        Serial.println(F("Motor detenido por comando."));
        return;
    }

    switch (sysMode) {
        case SystemMode::CALIBRATION:
            procesarComandoCal();
            break;
        case SystemMode::TUNING:
            if (strcmp(cmdBuffer, "STOP") == 0) {
                detener();
                tuneCtx.phase = TunePhase::IDLE;
                sysMode = SystemMode::OPERATION;
                Serial.println(F("TUNE abortado por usuario."));
            } else if (strcmp(cmdBuffer, "SAVEPID") == 0) {
                if (tuneCtx.phase == TunePhase::SAVE_PENDING) {
                    cfg.cv = MAGIC_NUMBER;
                    if (!eepromPutSafe(0, cfg)) { Serial.println(F("EEPROM write failed!")); }
                    Serial.println(F("PID guardado en EEPROM (sin validar amplitud)."));
                    Serial.println(F("Envie CAL para recalibrar limites, o RST para operacion normal."));
                    tuneCtx.phase = TunePhase::IDLE;
                    sysMode = SystemMode::OPERATION;
                    calState = CalState::IDLE;
                } else {
                    Serial.println(F("SAVEPID solo valido tras aborto por amplitud."));
                }
            } else {
                Serial.println(F("En TUNE solo se acepta STOP o SAVEPID."));
            }
            break;
        case SystemMode::OPERATION:
            break;
    }
}

static void trimWhitespace(char* str) {
    char* end = str + strlen(str) - 1;
    while (end >= str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        --end;
    }
    char* start = str;
    while (*start == ' ' || *start == '\t') { ++start; }
    if (start != str) { memmove(str, start, strlen(start) + 1); }
}

void procesarSerial() {
    static uint32_t lastCharMs = 0;
    while (Serial.available() > 0) {
        char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (cmdIndex > 0) {
                cmdBuffer[cmdIndex] = '\0';
                ucase(cmdBuffer);
                trimWhitespace(cmdBuffer);
                procesarComando();
                cmdIndex = 0;
            }
        } else if (cmdIndex < (sizeof(cmdBuffer) - 1u)) {
            cmdBuffer[cmdIndex] = c;
            ++cmdIndex;
            lastCharMs = millis();
        }
    }
    // Timeout de 5ms en CUALQUIER modo para procesar sin Enter
    if (cmdIndex > 0 && millis() - lastCharMs > 5) {
        cmdBuffer[cmdIndex] = '\0';
        ucase(cmdBuffer);
        trimWhitespace(cmdBuffer);
        procesarComando();
        cmdIndex = 0;
    }
}

void setup() {
    MCUSR = 0;
    wdt_disable();

    Serial.begin(115200);
    configurarPinesSinUso();

    initMotorHardware();

    if (!eepromGetSafe(0, cfg)) { Serial.println(F("EEPROM read failed!")); }

    filterPedal.init(static_cast<float>(analogRead(pinPotOp)));
    filterFeedback.init(static_cast<float>(analogRead(pinPotFeed)));
    filterCurrent.init(static_cast<float>(analogRead(PIN_IS_SENSE)));

    lastPidTime = micros();

    pinMode(A2, INPUT);
    oc_loadCalibration();
    if (!oc_isCalibrated()) {
        oc_calibrate();
        uint32_t ocTimeout = millis() + 5000;
        while (!oc_isCalibrated() && millis() < ocTimeout) {
            oc_updateCalibration();
            delayMicroseconds(100);
        }
        if (!oc_isCalibrated()) {
            Serial.println(F("[OC] Timeout calibracion. Sensor A2 desconectado?"));
        }
    }

    if (cfg.cv != MAGIC_NUMBER) {
        cfg = Config{};
        cfg.pMax = 1023;
        cfg.mMax = 1023;
        cfg.accelIsFwd = true;
        cfg.kp = DEFAULT_KP;
        cfg.ki = DEFAULT_KI;
        cfg.kd = DEFAULT_KD;
        iniciarCalibracion();
    } else {
        Serial.println(F("Sistema listo. Comandos: CAL RST TUNE"));
        sysMode = SystemMode::OPERATION;
    }

    wdt_enable(WDTO_2S);
}

void loop() {
    procesarSerial();

    filterPedal.update(static_cast<float>(analogRead(pinPotOp)));
    filterFeedback.update(static_cast<float>(analogRead(pinPotFeed)));

    if (deadTimeActive) {
        integralAccumulator = 0.0f;
        errorAnterior = 0;
    }

    static uint32_t lastOcCheck = 0;
    uint32_t now = millis();
    if (now - lastOcCheck >= 5) {
        lastOcCheck = now;
        if (oc_isCalibrated()) {
            oc_checkOverCurrent();
        }
    }

    oc_updateCalibration();

    switch (sysMode) {
        case SystemMode::CALIBRATION:
            tickCalibration();
            break;
        case SystemMode::TUNING:
            tickTuning();
            break;
        case SystemMode::OPERATION:
            tickOperation();
            break;
    }

    monitorStallCurrent();
    wdt_reset();
}