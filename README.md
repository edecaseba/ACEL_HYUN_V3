# ACEL_HYUN_V3 — Controlador de Acelerador para Excavadora Hyundai R250 LC7

Controlador electrónico de acelerador para excavadora Hyundai R250 LC7 con puente H IBT-2 (BTS7960), control PID en lazo cerrado y cumplimiento de normativas de seguridad industrial ISO 13849, ISO 7637-2 e ISO 13766.

**Versión actual: v2.0.21 (2026-07-16)**

---

## Especificaciones de Hardware

| Parámetro | Valor |
|-----------|-------|
| MCU | Arduino Nano (ATmega328P) @ 16 MHz |
| Framework | Arduino 2.3.0 |
| Alimentación | 24 Vcc |
| Fuente | Módulo Step-Down XL4005 (24V → 5V) |
| Flash | 32 KB |
| SRAM | 2 KB |
| Política de memoria | Zero Dynamic RAM (solo estática post-init) |
| Driver de potencia | Módulo IBT-2 BTS7960 (Puente H, 43A) |
| PWM | 20 kHz con dead-time configurable |
| Estándar de código | MISRA C:2012 / MISRA C++:2008 |

## Pinout

| Pin | Función | Descripción |
|-----|---------|-------------|
| A0 | PotOp | Potenciómetro de operación (pedal) |
| A1 | PotFeed | Potenciómetro de retroalimentación (acelerador) |
| A2 | IS_SENSE | Sensor de corriente (detección de stall) |
| D8 | EN | Enable del IBT-2 (R_EN + L_EN) |
| D9 | L_PWM | PWM de retroceso |
| D10 | R_PWM | PWM de avance |

## Arquitectura del Sistema

```
┌──────────┐    ┌──────────────┐    ┌───────────┐    ┌──────────┐
│ Pedal    │───►│ Filtro EMA   │───►│ PID      │───►│ IBT-2   │──► Motor
│ (ADC)    │    │ Anti-aliasing│    │ Controller│    │ (PWM)    │
└──────────┘    └──────────────┘    └───────────┘    └──────────┘
                       ▲                                  │
                       │                                  ▼
                       │                           ┌──────────┐
                       └───────────────────────────┤ Feedback │
                                                   │ (ADC)    │
                                                   └──────────┘
```

- **Filtro EMA**: Suavizado de lecturas analógicas para cumplir ISO 7637-2 (transitorios automotrices)
- **PID**: Control discreto con anti-windup, límite de slew rate y reset por fricción
- **IBT-2 (BTS7960)**: Dead-time obligatorio de 150 ms entre cambios de dirección (anti shoot-through)
- **Safe State**: Ante pérdida de señal o sobrecorriente, EN → LOW, motor detenido

**Calibración interactiva**: Los comandos de calibrado (`OK`, `SAVE`, `DIR`, `MOVEFWD`, `MOVEREV`, `SETMAX`, `SETMIN`, `FWD`, `REV`, `STOP`) son insensibles a mayúsculas/minúsculas. En modo calibrado el firmware procesa comandos tras 5 ms de inactividad sin requerir Enter. Durante cada paso se muestra el valor del sensor correspondiente (pedal o feedback).

## Estructura del Proyecto

```
ACEL_HYUN_V3/
├── src/
│   ├── main.cpp              # Lógica principal con PID integral
│   ├── overcurrent.cpp       # Detección overcurrent con auto-calibrado
│   ├── overcurrent.h
│   ├── pid_controller.h      # Controlador PID discreto
│   └── motor_types.h         # Tipos compartidos (enum ActuatorDirection)
├── test/
│   ├── test_pid.cpp          # Tests unitarios PID (Unity)
│   ├── test_overcurrent.cpp  # Tests unitarios overcurrent (Unity)
│   ├── test_runner.cpp       # Runner de tests
│   └── TEST_PROCEDURE.md     # Procedimiento de prueba en hardware
├── platformio.ini            # Build configuration
├── gitea-init.sh             # Script de inicialización Gitea
├── CHANGELOG.md              # Historial de versiones
├── AGENTS.md                 # Reglas del equipo de agentes
└── README.md                 # Este archivo
```

## Seguridad Funcional

- ✅ **ISO 13849**: Safe State ante fallos críticos (pérdida de sensor, timeout, sobrecorriente)
- ✅ **ISO 7637-2**: Filtrado digital (EMA) para transitorios automotrices de 24 Vcc
- ✅ **ISO 13766**: PWM a 20 kHz minimizando EMI, cambios de dirección con rampa y dead-time
- ✅ **Boot Security**: Pines de potencia configurados antes de activar enable
- ✅ **Zero Dynamic RAM**: Sin fragmentación de memoria en runtime

## Novedades v2.0.21 (2026-07-16)

### Auto-tuning PID robusto para actuadores lentos
- Timeout extendido **30s → 90s** (permite completar 6 ciclos en actuadores lentos)
- PWM tuning **140 → 180** (más torque para vencer fricción estática)
- Histeresis relay **±5 → ±3** (conmutación más rápida, oscilación más limpia)
- Setpoint adaptativo: usa centro real del rango calibrado (mMin+mMax)/2, no 50% fijo
- Progreso cada 5s: log de posición/error durante tuning para diagnóstico
- Ciclos mínimos **6 → 4** (más tolerante, validación de amplitud >1.0 mantenida)
- Timeout INIT_MOVE **5s → 10s** (más tiempo para alcanzar posición media)

### Fixes críticos
- **Serial timeout 5ms en TODOS los modos** (OPERATION, CALIBRATION, TUNING): procesa comandos sin requerir Enter
- **Dead-time en primer movimiento**: flag `firstMovementAfterStop` salta el bloqueo 150ms al iniciar desde parado
- **Overcurrent**: validación nominal=0/saturado, timeout 5s en setup, OCAL verifica motor parado

## Comandos Serie

| Comando | Descripción |
|---------|-------------|
| `CAL` | Iniciar calibración interactiva (6 pasos) |
| `OK` | Confirmar posición actual en calibración |
| `FWD` / `REV` / `STOP` | Probar dirección motor (paso 3/6) |
| `DIR FWD ACEL` / `DIR REV ACEL` | Configurar dirección de aceleración |
| `MOVEFWD` / `MOVEREV` | Mover a tope de aceleración/desaceleración |
| `SETMAX` / `SETMIN` | Guardar límites de feedback |
| `SAVE` | Guardar calibración en EEPROM |
| `TUNE` | Auto-tuning PID por relay (Åström-Hägglund) |
| `RST` | Resetear fault (overcurrent/stall) |
| `OCAL` | Recalibrar sensor de corriente (A2) |

## Requisitos de Desarrollo

- **IDE**: VS Code + PlatformIO
- **Toolchain**: AVR GCC (PlatformIO)
- **Lenguaje**: C++17 / C++20
- **Tests**: Unity Test Framework + ArduinoFake

## Build y Test

```bash
# Compilar
pio run -e nanoatmega328

# Tests unitarios (30/30 PASS)
pio test -e native

# Subir a Arduino Nano
pio run -e nanoatmega328 -t upload
```

## Métricas v2.0.21

- **RAM**: 27.3% (560/2048 bytes)
- **Flash**: 50.1% (15398/30720 bytes)
- **Tests**: 30/30 PASS (17 PID + 2 dead-time + 11 overcurrent)

## Licencia

Uso interno — maquinaria industrial pesada.
