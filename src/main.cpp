#include <Arduino.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <stdint.h>

// ==========================================
// --- Hardware Invariants: Topología BTS7960 ---
// ==========================================
constexpr uint8_t pinPotOp     = A0;
constexpr uint8_t pinPotFeed   = A1;
constexpr uint8_t PIN_IS_SENSE = A2;

constexpr uint8_t PIN_EN       = 8;
constexpr uint8_t PIN_R_PWM    = 10;
constexpr uint8_t PIN_L_PWM    = 9;

constexpr bool INVERTIR_GIRO_MOTOR = false;
constexpr uint16_t DEAD_TIME_MS = 150;
constexpr uint16_t STALL_CURRENT_ADC = 750;

// ==========================================
// --- Filtro EMA (Exponential Moving Average) ---
// ==========================================
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

constexpr float ALPHA_PEDAL    = 0.15f;
constexpr float ALPHA_FEEDBACK = 0.20f;
constexpr float ALPHA_CURRENT  = 0.30f;

static EMAFilter filterPedal    = { ALPHA_PEDAL, 0.0f };
static EMAFilter filterFeedback = { ALPHA_FEEDBACK, 0.0f };
static EMAFilter filterCurrent  = { ALPHA_CURRENT, 0.0f };

// ==========================================
// --- Config / EEPROM ---
// ==========================================
struct Config {
    int16_t pMin;
    int16_t pMax;
    int16_t mMin;
    int16_t mMax;
    int16_t cv;
};
static Config cfg;
constexpr int16_t MAGIC_NUMBER = 111;

static char cmdBuffer[16];
static uint8_t cmdIndex = 0;

// ==========================================
// --- Parametros de Control ---
// ==========================================
constexpr float KP = 4.0f;
constexpr float KD = 1.5f;
constexpr uint8_t ZONA_MUERTA = 3;
constexpr uint8_t PWM_MIN_OPERACION = 90;
static int16_t errorAnterior = 0;

// ==========================================
// --- Maquina de Estados ---
// ==========================================
enum class ActuatorDirection : uint8_t {
    STOP,
    FORWARD,
    REVERSE
};

struct MotorState {
    ActuatorDirection currentDirection;
    bool isFaulted;
};
static MotorState sysState = { ActuatorDirection::STOP, false };

static uint32_t deadTimeUntil = 0;
static bool deadTimeActive = false;

enum class CalState : uint8_t {
    IDLE,
    WAIT_PEDAL_MIN,
    READ_PEDAL_MIN,
    WAIT_PEDAL_MAX,
    READ_PEDAL_MAX,
    CAL_MOTOR_MIN,
    RUN_MOTOR_MIN,
    CAL_MOTOR_MAX,
    RUN_MOTOR_MAX,
    SAVE
};
static CalState calState = CalState::IDLE;
static uint32_t calTimer = 0;
static uint32_t calPhaseStart = 0;
static int16_t ultimaLectura = -1;
static uint32_t tEstable = 0;

static uint32_t lastStallCheck = 0;
constexpr uint16_t STALL_CHECK_INTERVAL_MS = 50;

// ==========================================
// --- Capa de Potencia (BTS7960) ---
// ==========================================

static void detener() {
    digitalWrite(PIN_R_PWM, LOW);
    digitalWrite(PIN_L_PWM, LOW);
    sysState.currentDirection = ActuatorDirection::STOP;
}

void initMotorHardware() {
    digitalWrite(PIN_EN, LOW);
    digitalWrite(PIN_R_PWM, LOW);
    digitalWrite(PIN_L_PWM, LOW);

    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_R_PWM, OUTPUT);
    pinMode(PIN_L_PWM, OUTPUT);
    pinMode(PIN_IS_SENSE, INPUT);

    TCCR1A = static_cast<uint8_t>(1 << WGM10);
    TCCR1B = static_cast<uint8_t>(1 << CS10);

    digitalWrite(PIN_EN, HIGH);
}

void mover(uint8_t vel, bool fwd_intent) {
    if (sysState.isFaulted) { return; }

    bool fwdEfectivo = fwd_intent ^ INVERTIR_GIRO_MOTOR;
    ActuatorDirection targetDir = fwdEfectivo ? ActuatorDirection::FORWARD : ActuatorDirection::REVERSE;

    if (targetDir != sysState.currentDirection && sysState.currentDirection != ActuatorDirection::STOP) {
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

    if (targetDir == ActuatorDirection::FORWARD) {
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
        Serial.println(F("CRITICAL: STALL DETECTADO. Puente H bloqueado."));
    }
}

// ==========================================
// --- Calibracion No Bloqueante ---
// ==========================================

static void iniciarCalibracion() {
    sysState.isFaulted = false;
    digitalWrite(PIN_EN, HIGH);
    detener();
    calState = CalState::WAIT_PEDAL_MIN;
    calTimer = millis();
    Serial.println(F("\n=== RUTINA DE CALIBRACION DE PRECISION ==="));
}

static void tickCalibration() {
    uint32_t now = millis();

    switch (calState) {
        case CalState::IDLE:
            break;

        case CalState::WAIT_PEDAL_MIN:
            Serial.println(F("1/3: SOLTA EL PEDAL (Reposo absoluto)..."));
            calPhaseStart = now;
            calState = CalState::READ_PEDAL_MIN;
            break;

        case CalState::READ_PEDAL_MIN:
            if (now - calPhaseStart >= 4000) {
                cfg.pMin = static_cast<int16_t>(filterPedal.filteredValue);
                Serial.print(F("MIN = ")); Serial.println(cfg.pMin);
                calState = CalState::WAIT_PEDAL_MAX;
            }
            break;

        case CalState::WAIT_PEDAL_MAX:
            Serial.println(F("2/3: APRETA EL PEDAL A FONDO (Mantenelo firme)..."));
            calPhaseStart = now;
            calState = CalState::READ_PEDAL_MAX;
            break;

        case CalState::READ_PEDAL_MAX:
            if (now - calPhaseStart >= 4000) {
                cfg.pMax = static_cast<int16_t>(filterPedal.filteredValue);
                Serial.print(F("MAX = ")); Serial.println(cfg.pMax);
                calState = CalState::CAL_MOTOR_MIN;
            }
            break;

        case CalState::CAL_MOTOR_MIN:
            Serial.println(F("3/3: MAPEANDO RECORRIDO DEL ACTUADOR..."));
            ultimaLectura = -1;
            tEstable = now;
            calPhaseStart = now;
            calState = CalState::RUN_MOTOR_MIN;
            break;

        case CalState::RUN_MOTOR_MIN: {
            mover(140, false);
            int16_t actual = static_cast<int16_t>(filterFeedback.filteredValue);
            if (abs(static_cast<int>(actual - ultimaLectura)) > 2) {
                tEstable = now;
                ultimaLectura = actual;
            }
            bool timeOut = (now - calPhaseStart > 10000);
            bool estable = (now - tEstable >= 1200);
            if (estable || timeOut) {
                detener();
                cfg.mMin = static_cast<int16_t>(filterFeedback.filteredValue);
                calState = CalState::CAL_MOTOR_MAX;
                calPhaseStart = now;
            }
            break;
        }

        case CalState::CAL_MOTOR_MAX:
            ultimaLectura = -1;
            tEstable = now;
            calPhaseStart = now;
            calState = CalState::RUN_MOTOR_MAX;
            break;

        case CalState::RUN_MOTOR_MAX: {
            mover(140, true);
            int16_t actual = static_cast<int16_t>(filterFeedback.filteredValue);
            if (abs(static_cast<int>(actual - ultimaLectura)) > 2) {
                tEstable = now;
                ultimaLectura = actual;
            }
            bool timeOut = (now - calPhaseStart > 10000);
            bool estable = (now - tEstable >= 1200);
            if (estable || timeOut) {
                detener();
                cfg.mMax = static_cast<int16_t>(filterFeedback.filteredValue);
                calState = CalState::SAVE;
            }
            break;
        }

        case CalState::SAVE:
            if (abs(static_cast<int>(cfg.mMax - cfg.mMin)) > 50) {
                cfg.cv = MAGIC_NUMBER;
                EEPROM.put(0, cfg);
                Serial.println(F("=== HARDWARE CALIBRADO EXITOSAMENTE ==="));
            } else {
                Serial.println(F("FATAL: Actuador no registra movimiento. Verifique acople."));
            }
            calState = CalState::IDLE;
            break;
    }
}

// ==========================================
// --- I/O Serial ---
// ==========================================

void procesarSerial() {
    while (Serial.available() > 0) {
        char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (cmdIndex > 0) {
                cmdBuffer[cmdIndex] = '\0';
                if (strstr(cmdBuffer, "CAL") != nullptr) {
                    cfg.cv = 0;
                    EEPROM.put(0, cfg);
                    Serial.println(F("Iniciando recalibracion..."));
                    iniciarCalibracion();
                } else if (strstr(cmdBuffer, "RST") != nullptr) {
                    sysState.isFaulted = false;
                    digitalWrite(PIN_EN, HIGH);
                    Serial.println(F("Fallo de Stall reseteado."));
                }
                cmdIndex = 0;
            }
        } else if (cmdIndex < (sizeof(cmdBuffer) - 1u)) {
            cmdBuffer[cmdIndex] = c;
            ++cmdIndex;
        }
    }
}

// ==========================================
// --- Setup & Loop ---
// ==========================================

void setup() {
    MCUSR = 0;
    wdt_disable();

    Serial.begin(115200);

    initMotorHardware();

    EEPROM.get(0, cfg);

    filterPedal.init(static_cast<float>(analogRead(pinPotOp)));
    filterFeedback.init(static_cast<float>(analogRead(pinPotFeed)));
    filterCurrent.init(static_cast<float>(analogRead(PIN_IS_SENSE)));

    if (cfg.cv != MAGIC_NUMBER) {
        iniciarCalibracion();
    } else {
        Serial.println(F("Sistema en Operacion. (Envie 'CAL' para reconfigurar, 'RST' para clear fault)"));
    }

    wdt_enable(WDTO_2S);
}

void loop() {
    procesarSerial();

    filterPedal.update(static_cast<float>(analogRead(pinPotOp)));
    filterFeedback.update(static_cast<float>(analogRead(pinPotFeed)));

    tickCalibration();
    monitorStallCurrent();

    if (calState == CalState::IDLE && cfg.cv == MAGIC_NUMBER && !sysState.isFaulted) {
        int16_t rOp = static_cast<int16_t>(filterPedal.filteredValue);
        int16_t rFe = static_cast<int16_t>(filterFeedback.filteredValue);

        int16_t setpoint = map(rOp, cfg.pMin, cfg.pMax, 0, 100);
        int16_t posicion = map(rFe, cfg.mMin, cfg.mMax, 0, 100);

        setpoint = constrain(setpoint, 0, 100);
        posicion = constrain(posicion, 0, 100);

        int16_t errorActual = static_cast<int16_t>(setpoint - posicion);

        if (abs(static_cast<int>(errorActual)) > static_cast<int16_t>(ZONA_MUERTA)) {
            bool direccionHaciaMax = (errorActual > 0);

            if (direccionHaciaMax && (rFe >= cfg.mMax)) {
                detener();
            } else if (!direccionHaciaMax && (rFe <= cfg.mMin)) {
                detener();
            } else {
                int16_t derivada = static_cast<int16_t>(errorActual - errorAnterior);
                int16_t salidaPD = static_cast<int16_t>(
                    (static_cast<float>(errorActual) * KP) +
                    (static_cast<float>(derivada) * KD)
                );
                uint8_t vel = static_cast<uint8_t>(
                    constrain(
                        abs(salidaPD),
                        static_cast<int16_t>(PWM_MIN_OPERACION),
                        static_cast<int16_t>(255)
                    )
                );
                mover(vel, direccionHaciaMax);
            }
        } else {
            detener();
        }

        errorAnterior = errorActual;

        static uint32_t t = 0;
        if (millis() - t > 100) {
            Serial.print(F("SetP:")); Serial.print(setpoint);
            Serial.print(F("% ActP:")); Serial.print(posicion);
            Serial.print(F("% Err:")); Serial.println(errorActual);
            t = millis();
        }
    }

    wdt_reset();
}
