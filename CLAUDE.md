# ACEL_HYUN_V3

Controlador de acelerador para excavadora Hyundai R250 LC7: lee un pedal (potenciometro), lo mapea contra un feedback de posicion del actuador, y mueve un motor DC via puente H (IBT-2/BTS7960) con control PID. Firmware de seguridad funcional para maquinaria pesada real — tratar cada cambio con ese criterio.

## Idioma y tono
Responder siempre en **castellano**, directo, sin saludos ni frases de cortesia innecesarias.

## Hardware — unico y definitivo
**Arduino Nano — ATmega328P**, framework Arduino/PlatformIO. Detalle completo (pines, memoria, reglas de codificacion) en `ai/hardware_target.json` — es la unica fuente de verdad, no crear copias.

Pines clave: `PIN_POT_OP` A0, `PIN_POT_FEED` A1, `PIN_IS_SENSE` A2, `PIN_EN` D8, `PIN_L_PWM` D9 (OC1A), `PIN_R_PWM` D10 (OC1B).

**El actuador NO tiene finales de carrera fisicos.** El unico limite de posicion es software: el rango calibrado del potenciometro de feedback (`cfg.mMin`/`cfg.mMax`) mas la deteccion de stall por sobrecorriente contra el tope mecanico real. Los potenciometros (pedal y feedback) son divisores resistivos simples sin pull-up/pull-down: un cable cortado o wiper desconectado puede flotar a cualquier valor ADC, no a un riel conocido. Por eso `checkSignalLoss()` en `main.cpp` (logica pura en `src/signal_loss.h`) dispara Safe State si una lectura queda fuera del rango calibrado — es la unica defensa que existe, tenerlo presente en cualquier cambio a la logica de posicion/calibracion.

**No hay ni habra migracion a Rust ni a ESP32-S3 (ni a ningun otro MCU).** Decision explicita del usuario: no es seguro migrar el control de un motor de maquinaria pesada real a un ecosistema/toolchain menos maduro en microcontroladores de esta clase. La placa ya esta fabricada con el ATmega328P. No proponer ni planificar ese tipo de migracion salvo que el usuario lo pida explicitamente de nuevo.

## Invariantes que no se rompen nunca
- Zero Dynamic RAM post-init (sin `String`, `new`, `malloc` en runtime)
- Nunca `delay()` en una ruta que corra con el watchdog activo o el motor habilitado — usar `millis()`/`micros()`
- Watchdog: `wdt_enable(WDTO_2S)` en `setup()`, `wdt_reset()` en cada `loop()`
- Dead-time de 150ms entre cambios de direccion del puente H
- PWM del puente H a 20kHz, nunca por encima de 25kHz (limite real del BTS7960 — ver skill `ibt2-bts7960`, incidente documentado en `motor-startup-diagnostics`)
- Direcciones EEPROM sin colisionar: `Config` (main.cpp, addr 0) vs calibracion de sobrecorriente (`overcurrent.h`, addr 64+) — hay un `static_assert` que lo protege, no correrlo por decoracion
- `checkSignalLoss()` debe seguir corriendo en cada `loop()` durante `OPERATION` — es el unico respaldo ante perdida de señal de un potenciometro, no hay finales de carrera fisicos

## Build y test
- `pio run -e nanoatmega328` — build real del firmware (AVR), debe compilar limpio con `-Werror`
- `pio test -e native` — suite Unity con mocks (`test/mock_arduino.*`), correr siempre antes de dar un cambio por terminado

## Skills (cargar segun corresponda, no todas de una)
- `embedded-safety-rules` — MISRA/ISO/memoria/Safe State, cargar antes de tocar cualquier archivo en `src/`
- `ibt2-bts7960` — specs del driver de motor, cargar al tocar PWM/puente H/corriente
- `motor-startup-diagnostics` — causas raiz ya investigadas de fallas de arranque (reset del micro, PWM sin autoridad, EEPROM corrupta) — revisar ANTES de re-investigar un sintoma de arranque desde cero

## Pipeline de agentes (`.claude/agents/`)
Para cambios no triviales en `src/`, seguir: `fw-planner` → `fw-coder` → `fw-reviewer` (veto absoluto) → `fw-tester` → `fw-documenter`. Para busquedas rapidas, `fw-explorer`. Para calibracion de sensores/actuadores, `calibrator`. No hace falta pasar por el pipeline completo para preguntas o exploracion — el pipeline es para cambios de codigo reales.

Excepcion: emergencias de hardware critico (motor no gira, safe-state falla) se pueden resolver directo y documentar despues via `fw-documenter` — no bloquear un hotfix real esperando el pipeline completo.

## Historial
No hay pipeline de `opencode` en este proyecto — se migro por completo a subagentes nativos de Claude Code (`.claude/agents/`) y skills (`.claude/skills/`). `CHANGELOG.md` tiene el historial tecnico version a version.
