# ACEL_HYUN_V3 — Controlador de Acelerador para Excavadora Hyundai R250 LC7

Controlador electrónico de acelerador para excavadora Hyundai R250 LC7 con puente H IBT-2 (BTS7960), control PID en lazo cerrado y cumplimiento de normativas de seguridad industrial ISO 13849, ISO 7637-2 e ISO 13766.

**Versión actual: v2.0.24 (2026-08-17)**

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
│   ├── test_main.cpp         # Tests unitarios de la maquina de estados (Unity)
│   ├── test_runner.cpp       # Runner de tests
│   ├── mock_arduino.h/.cpp   # Mocks de Arduino/EEPROM para el entorno native
│   └── TEST_PROCEDURE.md     # Procedimiento de prueba en hardware
├── ai/hardware_target.json   # Fuente de verdad de hardware (pines, memoria, reglas)
├── .claude/agents/           # Subagentes Claude Code (planner/coder/reviewer/tester/...)
├── .claude/skills/           # Conocimiento persistente (MISRA/ISO, IBT-2, diagnostico arranque)
├── CLAUDE.md                 # Instrucciones del proyecto para Claude Code
├── platformio.ini            # Build configuration
├── gitea-init.sh             # Script de inicialización Gitea
├── CHANGELOG.md              # Historial de versiones
├── CALIBRACION_GUIA.md       # Guía paso a paso de calibración (manual y ACAL)
└── README.md                 # Este archivo
```

## Seguridad Funcional

- ✅ **ISO 13849**: Safe State ante fallos críticos (pérdida de sensor, timeout, sobrecorriente)
- ✅ **ISO 7637-2**: Filtrado digital (EMA) para transitorios automotrices de 24 Vcc
- ✅ **ISO 13766**: PWM a 20 kHz minimizando EMI, cambios de dirección con rampa y dead-time
- ✅ **Boot Security**: Pines de potencia configurados antes de activar enable
- ✅ **Zero Dynamic RAM**: Sin fragmentación de memoria en runtime

## Novedades v2.0.24 (2026-08-17)

### Fix crítico: reset del micro al arrancar el motor
Ver `CHANGELOG.md` para el detalle completo. En resumen: el PWM del puente H se había subido por error a 62.5kHz (el IBT-2/BTS7960 admite máximo 25kHz), había un bug de modo en el Timer1 (OCR1A hacía de TOP y de duty al mismo tiempo) y una colisión de direcciones EEPROM entre la config general y la calibración de sobrecorriente. Los tres se corrigieron, más un soft-start no bloqueante y diagnóstico de causa de reset (`MCUSR`) por Serial.

### v2.0.23 — Auto-calibración completa (ACAL)
Comando `ACAL`: calibra el pedal igual que `CAL` pero busca los topes mecánicos del actuador sola, usando detección de sobrecorriente. Ver `CALIBRACION_GUIA.md` sección 4 para el procedimiento completo.

### v2.0.21 — Auto-tuning PID robusto para actuadores lentos
- Timeout extendido **30s → 90s** (permite completar 6 ciclos en actuadores lentos)
- PWM tuning **140 → 180** (más torque para vencer fricción estática)
- Histeresis relay **±5 → ±3** (conmutación más rápida, oscilación más limpia)
- Setpoint adaptativo: usa centro real del rango calibrado (mMin+mMax)/2, no 50% fijo

## Comandos Serie

| Comando | Descripción |
|---------|-------------|
| `CAL` | Iniciar calibración manual interactiva (6 pasos) |
| `ACAL` | Auto-calibración: pedal manual + topes del actuador automáticos por overcurrent |
| `OK` | Confirmar posición actual en calibración |
| `FWD` / `REV` / `STOP` | Probar dirección motor (paso 3/6 de `CAL`) |
| `DIR FWD ACEL` / `DIR REV ACEL` | Configurar dirección de aceleración |
| `MOVEFWD` / `MOVEREV` | Mover a tope de aceleración/desaceleración |
| `SETMAX` / `SETMIN` | Guardar límites de feedback |
| `SAVE` | Guardar calibración en EEPROM |
| `TUNE` | Auto-tuning PID por relay (Åström-Hägglund) |
| `SAVEPID` | Guardar PID de `TUNE` sin validar |
| `RST` | Resetear fault (overcurrent/stall) |
| `OCAL` | Recalibrar sensor de corriente (A2) |

Guía completa paso a paso: `CALIBRACION_GUIA.md`.

## Requisitos de Desarrollo

- **IDE**: VS Code + PlatformIO
- **Toolchain**: AVR GCC (PlatformIO)
- **Lenguaje**: C++17 / C++20
- **Tests**: Unity Test Framework + ArduinoFake

## Build y Test

```bash
# Compilar
pio run -e nanoatmega328

# Tests unitarios (38/38 PASS)
pio test -e native

# Subir a Arduino Nano
pio run -e nanoatmega328 -t upload
```

## Métricas v2.0.24

- **RAM**: 29.0% (594/2048 bytes)
- **Flash**: 59.1% (18160/30720 bytes)
- **Tests**: 38/38 PASS (17 PID + 2 dead-time + 11 overcurrent + 8 maquina de estados)

## Horas de Desarrollo

| Fase | Horas Estimadas |
|------|-----------------|
| Arquitectura y diseño (HW/SW) | 12 h |
| PID discreto + anti-windup + dead-time | 16 h |
| Filtro EMA + ISO 7637-2 compliance | 8 h |
| Calibración interactiva (6 pasos) | 20 h |
| Auto-tuning PID (relay + limit cycle) | 24 h |
| Overcurrent detection + auto-calibración | 12 h |
| Safe State + Watchdog + ISO 13849 | 10 h |
| Tests Unity (30 tests) + CI | 14 h |
| Documentación (README, CHANGELOG, guías) | 8 h |
| Debugging hardware + validación | 18 h |
| **TOTAL** | **~148 horas** |

## Licencia

Uso interno — maquinaria industrial pesada.
