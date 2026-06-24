# CHANGELOG — ACEL_HYUN_V3
## ESTADO ACTUAL (2026-06-20)
**v1.5.0 — MCU:** Arduino Nano ATmega328P · PlatformIO · Arduino Framework
**Target alternativo:** ESP32-S3 @240MHz · ESP-IDF v5.5.3 · FreeRTOS · C++20

### Archivos base
`src/main.cpp` `src/pid_controller.h` `test/test_pid.cpp` `test/TEST_PROCEDURE.md`
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
