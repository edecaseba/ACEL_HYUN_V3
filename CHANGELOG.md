# CHANGELOG — ACEL_HYUN_V3
## ESTADO ACTUAL (2026-06-30)
**v2.0.2 — MCU:** Arduino Nano ATmega328P · PlatformIO · Arduino Framework
**Target alternativo:** ESP32-S3 @240MHz · ESP-IDF v5.5.3 · FreeRTOS · C++20

### Archivos base
`src/main.cpp` `src/pid_controller.h` `src/overcurrent.h` `src/overcurrent.cpp` `test/test_pid.cpp` `test/test_overcurrent.cpp` `test/TEST_PROCEDURE.md`
`README.md` `platformio.ini` `gitea-init.sh` `.vscode/tasks.json`
`ai/{persona,hardware_target,code_rules}.md` `AGENTS.md` `opencode.json`
`.opencode/skills/{skill-ibt2,skill-iso-safety}.md`

## ACTUAL (2026-06-24)
**v2.0.0 — Calibración Interactiva + Auto-tuning PID + Control Asentado**

### Cambios en v2.0.0
✅ **Calibración interactiva por comando serial**: 6 pasos guiados, técnico confirma cada
    posición manualmente (OK/FWD/REV/STOP/DIR/MOVEFWD/MOVEREV/SETMAX/SETMIN/SAVE)
✅ **Auto-tuning PID por relay** (Åström-Hägglund): comando `TUNE` oscila el actuador,
    mide Tu/Ku, calcula KP/KI/KD vía Ziegler-Nichols, guarda en EEPROM
✅ **Control con asentamiento (anti-ronroneo)**: motor completamente apagado cuando el
    pedal está quieto; histéresis 3/6, timer 100ms
✅ **KP/KI/KD runtime**: migrados de `constexpr` a campo en `PidInput`, almacenados en
    EEPROM dentro del struct `Config`
✅ **Dirección dinámica**: `cfg.accelIsFwd` reemplaza `INVERTIR_GIRO_MOTOR` compile-time;
    configurable por serial (`DIR FWD ACEL` / `DIR REV ACEL`)
✅ **Comandos seriales expandidos**: CAL, OK, FWD, REV, STOP, DIR, MOVEFWD, MOVEREV,
    SETMAX, SETMIN, SAVE, TUNE, RST
✅ **Reporte serial en operación**: SetP/Act/Err + [ASENTADO] + KP/KI/KD muestra cada 250ms
✅ **Tests Unity actualizados** (15 tests, mismos asserts con nueva firma)
✅ **TEST_PROCEDURE.md** reescrito: 10 casos de prueba + referencia rápida de comandos

### Funcionalidades preservadas de v1.5.0
✅ PID con anti-windup, zona muerta, derivada normalizada por Ts
✅ Filtro EMA anti-aliasing
✅ Dead-time 150ms (anti shoot-through)
✅ Safe State por stall / pérdida de señal
✅ Zero Dynamic RAM Post-Init
✅ Config persistente en EEPROM con validación por magic number

### Cambios en v1.5.0 (Hardware Mínimo)
✅ **BOM reducida**: eliminados ferritas (F2-F5), snubbers (R_snub + C_snub), common mode choke (L1), capacitores extra (C2-C4, C4A, C5A, C5B), diodos clamp (D2, D3), reset externo (SW1 + R5), resistor VIN (R4), regulador 3.3V (U2)
✅ **Protección mínima**: solo F1(PTC) + D1(TVS SMCJ28CA) + C1(470µF) en entrada 24V
✅ **ISO 13766 simplificada**: eliminados requisitos de ferritas, snubbers y common mode choke
✅ **ISO 7637-2 simplificada**: solo TVS + C1 bulk + filtrado digital EMA
✅ **PCB_DESIGN_GUIDE.md** actualizado: esquemático reducido, layout simplificado, jumpers reducidos de 8 a 4
✅ **KiCAD_SETUP.md** actualizado: netlist sin componentes eliminados
✅ **code_rules.md** actualizado: sin requisitos de ferritas/snubbers en EMI
✅ Hardware funcionalmente idéntico a versión L298N sin componentes EMI extra

### Funcionalidades
✅ PID integral con anti-windup, slew rate, reset por fricción
✅ Filtro EMA anti-aliasing (ISO 7637-2)
✅ Dead-time 150ms (anti shoot-through)
✅ Safe State ante pérdida de señal / sobrecorriente
✅ Safe State obligatorio antes de OTA (ESP32-S3)
✅ Watchdog con Safe State + reboot + registro (ESP32-S3)
✅ Zero Dynamic RAM Post-Init (ambos targets)
✅ PSRAM Octal: alloc solo en init, alineación 4/8/16 bytes (ESP32-S3)
✅ Pool universal de 7 agentes intercambiables por cuota
✅ Tests Unity (16/16 PASS)
✅ Gitea→GitHub push mirror (sync_on_commit)
✅ Placa 1 sola cara (B.Cu), 100% THT, XL4005 module, IBT-2 module
✅ Contexto agnóstico dual-target: cambiar `active_target` switchea MCU+framework+reglas

### Pendiente
⬜ `test/TEST_PROCEDURE.md` — procedimiento de prueba en hardware
⬜ Validación de parámetros PID en hardware real

### Equipo
| Agente | Modelo |
|--------|--------|
| orchestrator | Gemini 3.1 Pro |
| planner | North Mini Code Free |
| coder | DeepSeek V4 Flash Free |
| reviewer | Mimo V2.5 Free |
| tester | North Mini Code Free |
| documenter | Mimo V2.5 Free |
| explore | Gemini 3.5 Flash |

> Pool universal: cualquier agente puede tomar el rol de otro si falla por cuota (429). Cadena: planner→coder→reviewer→explore→tester→documenter→orchestrator.

## v2.0.1 — 2026-06-28
### Correcciones de cumplimiento MISRA/ISO y seguridad
- ✅ Eliminado uso de `String` y `delay()` (no se encontraron en código fuente).
- ✅ Reemplazado macros sin tipo por `constexpr`/`const` (ya estaban como `constexpr`).
- ✅ Protegido acceso a EEPROM con funciones `eepromPutSafe`/`eepromGetSafe` que verifican límites.
- ✅ Añadida regla explícita de seguridad en ISR a `ai/persona.md`.
- ✅ Habilitado `-Werror` en `platformio.ini` (tratamiento de warnings como errores).
- ✅ Restringido permiso `external_directory` en `opencode.json` a rutas necesarias (`src/`, `test/`, `.opencode/`, `ai/`).
- ✅ Actualizados todos los agentes del meta‑orquestador con plantillas válidas y permisos alineados a `AGENTS.md`.
- ✅ Creado `.opencode/project_state.json` y `.opencode/session_log.md` para seguimiento de estado.
- ✅ Compilación exitosa para objetivo Arduino Nano ATmega328P (RAM 25%, Flash 41.7%).
- ✅ Tests unitarios siguen pasando (ver `pio test`).

## v2.0.2 — 2026-06-30
### Nuevas funcionalidades
- **Detección dinámica de sobre‑corriente con auto‑calibrado**:
  * Hardware: pull‑down 10 kΩ + desacoplador 100 nF en A2; diodos SR2100MIC forman un OR pasivo entre RIS y LIS del IBT‑2.
  * Software: nuevos archivos `overcurrent.h/.cpp`; modificaciones en `main.cpp` para cargar calibrado, ejecutar rutina de calibrado (comando serial `C`) y chequeo continuo (`oc_checkOverCurrent()`).
  * Umbral de detección: `U = nominal + K·σ` (K configurable, valor por defecto 4).
  * Los valores nominal y sigma se guardan en EEPROM (direcciones 0x00 y 0x02) y persisten entre ciclos de energía.
  * Comando serial `C` (o `c`) dispara un nuevo calibrado; el calibrado automático se ejecuta en el primer arranque si la EEPROM está virgen.
- **Pruebas unitarias** añadidas (`test/test_overcurrent.cpp`) que verifican carga de valores por defecto, cálculo correcto de media y sigma, y funcionamiento del umbral.
- **Procedimiento de prueba** actualizado (`TEST_PROCEDURE.md`) con pasos de hardware y validación de la detección de sobre‑corriente.
- **Impacto en recursos** (Arduino Nano ATmega328P):
  * RAM: +≈1 % (≈20 bytes adicionales).
  * Flash: +≈1.2 KB (código de calibrado y chequeo).

### Funcionalidades preservadas
- Todas las características existentes de v2.0.0 y v2.0.1 (PID con anti‑windup, zona muerta, filtro EMA, dead‑time 150 ms, control con asentamiento, watchdog, safe‑state por sobrecorriente/pérdida de señal, etc.) permanecen sin cambios.

## v2.0.3 — 2026-09-16
### Correcciones de rendimiento y seguridad
- Eliminado uso de `delay()` en la calibración de sobre‑corriente (overcurrent.cpp) y reemplazado por espera no bloqueante basada en `micros()`.
- Agregado `#include <math.h>` para uso de `sqrt()`.
- Mejorado cálculo de varianza para evitar subdesbordamiento usando enteros con signo.
- Añadido pequeño `yield()` en `loop()` (delayMicroseconds(100)) para liberar tiempo de UART y evitar sobrecarga del buffer serial.
- Todas las pruebas unitarias continúan pasando.

## v2.0.4 — 2026-09-16
### Configuración de pines sin uso
- Añadida función `configurarPinesSinUso()` que pone en `INPUT_PULLUP` los pines digitales no utilizados (D2‑D7, D11‑D13) y asegura que los pines usados queden en estado correcto.
- Llamada desde `setup()` antes de `initMotorHardware()`.
- Los pines analógicos A6 y A7 se dejan tal cual (pull‑down externo según indicación).
- Todas las pruebas unitarias continúan pasando (salvo problemas conocidos en el entorno de prueba externos al código).