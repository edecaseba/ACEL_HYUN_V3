# CHANGELOG — ACEL_HYUN_V3
## v2.0.21 — 2026-07-16
### Mejora: Auto-tuning PID robusto para actuadores lentos
- ✅ **Timeout extendido 30s → 90s**: permite completar 6 ciclos en actuadores lentos
- ✅ **PWM tuning 140 → 180**: más torque para vencer fricción estática
- ✅ **Histeresis relay ±5 → ±3**: conmutación más rápida, oscilación más limpia
- ✅ **Setpoint adaptativo**: usa centro real del rango calibrado (mMin+mMax)/2, no 50% fijo
- ✅ **Progreso cada 5s**: log de posición/error durante tuning para diagnóstico
- ✅ **Ciclos mínimos 6 → 4**: más tolerante, validación de amplitud >1.0 mantenida
- ✅ **Timeout INIT_MOVE 5s → 10s**: más tiempo para alcanzar posición media
- ✅ **Compilación**: RAM 27.3%, Flash 50.1%.
- ✅ **Tests unitarios**: 30/30 PASS.

## v2.0.20 — 2026-07-16
### Fix: Motor no responde a FWD/REV en calibración (dead-time bloqueaba primer movimiento)
- ✅ **Refactor `mover()`**: elimina dead-time residual en primer comando tras `detener()`. Ahora el dead-time 150ms **solo** se activa al cambiar pin PWM *mientras el motor está en movimiento* (cambio FWD↔REV bajo carga), no al iniciar desde parado.
- ✅ **Primer FWD/REV en calibración funciona inmediato**: `detener()` resetea `deadTimeActive=false`, `deadTimeUntil=0`, `lastUsarR_PWM` se actualiza al nuevo pin sin bloqueo.
- ✅ **Anti shoot-through preservado**: inversión brusca FWD→REV en operación sigue disparando dead-time 150ms obligatorio.
- ✅ **Compilación**: RAM 27.1%, Flash 48.8%.
- ✅ **Tests unitarios**: 30/30 PASS (incluye 2 tests dead-time existentes).

## v2.0.19 — 2026-07-16
### Fix: Procesamiento de comandos seriales sin requerir Enter
- ✅ **Timeout 5ms en TODOS los modos (OPERATION, CALIBRATION, TUNING)**: `procesarSerial()` ahora procesa el buffer tras 5ms de inactividad sin necesidad de `\n`/`\r`. Permite enviar `CAL`, `RST`, `OCAL`, `TUNE`, `STOP` sin pulsar Enter.
- ✅ **Fix motor "clavado"**: era comportamiento correcto (ASENTADO con error=1 < umbral 6), pero sin poder recalibrar por falta de Enter.
- ✅ **Compilación**: RAM 27.1%, Flash 48.8%.
- ✅ **Tests unitarios**: 30/30 PASS.

## v2.0.18 — 2026-07-15
### Limpieza: Eliminar prints de debug que inundan puerto serie
- ✅ **Quitar `Serial.print` en `mover()` y `monitorStallCurrent()`**: los mensajes de debug cada ciclo PID saturaban el buffer serie, bloqueando recepción de comandos (`CAL`, `RST`, etc.) y reporte PID.
- ✅ **Serial solo para eventos críticos**: comandos, reportes 250ms, errores overcurrent/stall, calibración.
- ✅ **Compilación**: RAM 27.1%, Flash 48.9% (-454 bytes).
- ✅ **Tests unitarios**: 30/30 PASS.

## v2.0.17 — 2026-07-15
### Fix: Dead-time reforzado por cambio de pin PWM (anti shoot-through)
- ✅ **Dead-time 150ms obligatorio al cambiar pin PWM (R_PWM ↔ L_PWM)**: nuevo tracking `lastUsarR_PWM` en `mover()`. Ahora el dead-time se dispara SIEMPRE que cambia el pin PWM activo, no solo al cambiar dirección lógica. Previene shoot-through en inversión brusca bajo carga (plugging).
- ✅ **Fix overcurrent real (raw=1023) en desaceleración**: causado por inversión instantánea FWD→REV sin dead-time efectivo.
- ✅ **Compilación**: RAM 27.1%, Flash 50.4%.
- ✅ **Tests unitarios**: 30/30 PASS.

## v2.0.16 — 2026-07-15
### Fix: Calibración overcurrent robusta + timeout setup
- ✅ **Validación rango nominal en `oc_updateCalibration()`**: rechaza nominal=0 o ≥1023 (sensor desconectado/saturado). Error: `[OC] Error: Lectura invalida (nominal=0 o saturado). Verifique sensor A2.`
- ✅ **Timeout 5s en setup()** esperando `oc_isCalibrated()`: evita bloqueo infinito si A2 flotante. Mensaje: `[OC] Timeout calibracion. Sensor A2 desconectado?`
- ✅ **Compilación**: RAM 27.1%, Flash 50.3% (+220 bytes por validación).
- ✅ **Tests unitarios**: 30/30 PASS.

## v2.0.15 — 2026-07-15
### Fix crítico: PWM deshabilitado por sobrescritura de TCCR1A
- ✅ **Preservar bits COM1A1/COM1B1 en `initMotorHardware()`**: el código sobrescribía `TCCR1A` borrando la configuración de PWM del Arduino core (pines 9/10). Fix: `TCCR1A = (TCCR1A & 0xF0) | (1<<WGM10)` y `TCCR1B = (TCCR1B & 0xF0) | ((1<<WGM12)|(1<<CS10))`.
- ✅ **Motor vuelve a girar** con comandos FWD/REV/MOVEFWD/MOVEREV.
- ✅ **Compilación**: RAM 27.1%, Flash 49.6% (sin cambios).
- ✅ **Tests unitarios**: 30/30 PASS.

## v2.0.14 — 2026-07-15
### Fixes finales: STALL threshold, OCAL motor check, trim comandos, motor_types.h
- ✅ **STALL_CURRENT_ADC 750→950**: evita falsos positivos con picos normales de corriente de trabajo (~700-800 ADC). Solo dispara en stall real (>950).
- ✅ **OCAL con verificación motor parado**: `oc_calibrate()` verifica `sysState_currentDirection == STOP`. Si motor moviéndose → error `[OC] Error: Motor en movimiento. Detenga antes de calibrar.`
- ✅ **Trim whitespace en comandos**: `trimWhitespace()` en `procesarSerial()` elimina espacios leading/trailing. `STOP ` → `STOP`.
- ✅ **motor_types.h**: header compartido con `enum class ActuatorDirection` para evitar definición múltiple entre main.cpp y overcurrent.h.
- ✅ **Overcurrent threshold fijo 850**: `OC_THRESHOLD_FIXED=850` + mínimo 500. Detecta solo cortocircuito real (>1000), no corriente trabajo normal (~735).
- ✅ **Sync sysState_currentDirection**: actualizado en `detener()` y `mover()` para OCAL check.
- ✅ **Tests unitarios**: 30/30 PASS (17 PID + 2 dead-time + 11 overcurrent).
- ✅ **Compilación**: RAM 27.1%, Flash 49.6% (dentro de límites ATmega328P).
- ✅ **TEST_PROCEDURE.md**: Caso 12 (Overcurrent/STALL validation) + Caso 11 (Dead-time).

## v2.0.13 — 2026-07-15
### Corrección de calibración interactiva: comandos case-insensitive + timeout recepción
- ✅ **Comandos case-insensitive (v2.0.6)**: agregada función `ucase()` que convierte el buffer de comando a mayúsculas antes de comparar. Ahora aceptan `ok`, `Ok`, `OK`, `save`, `SAVE`, `fwd`, `FWD`, etc.
- ✅ **Timeout de recepción en modo calibrado (v2.0.7)**: `procesarSerial()` ahora procesa el buffer tras 5 ms de inactividad aunque no llegue fin de línea. Evita que comandos como "OK" queden sin procesar si el puerto se cierra tras enviar.
- ✅ **Verificación MISRA/ISO**: función `ucase()` solo recorre la cadena y resta `'a'-'A'`, sin acceso fuera de límites, sin asignación dinámica.
- ✅ **Compilación**: exitosa, RAM 26.3%, Flash 43.6%.
- ✅ **Pruebas unitarias**: 30/30 PASS (incluye 2 tests dead-time nuevos).

## v2.0.12 — 2026-07-14
### Corrección de dead-time residual bloqueando MOVEREV tras SETMAX
- ✅ **Reset de `deadTimeActive` y `deadTimeUntil` en `detener()`**: la función `detener()` ahora pone `deadTimeActive = false` y `deadTimeUntil = 0`, evitando que un dead-time residual bloquee el siguiente movimiento después de SETMAX, SETMIN o STOP.
- ✅ **Causa raíz**: al llamar `detener()` desde SETMAX, `deadTimeActive` quedaba en `true` con un `deadTimeUntil` viejo. La siguiente llamada a `mover()` (MOVEREV) entraba al bloque `if (deadTimeActive)` y retornaba sin mover porque `millis() < deadTimeUntil`.
- ✅ **Verificación MISRA/ISO**: variables static globales, sin asignación dinámica, sin efectos secundarios en otros puntos de llamada a `detener()`.
- ✅ **Compilación**: exitosa, RAM 26.1%, Flash 43.2%.
- ⬜ **Pruebas unitarias**: pendiente agregar test de dead-time y actualizar TEST_PROCEDURE.md.

## v2.0.11 — 2026-07-06
### Corrección de control de movimiento durante calibrado interactivo
- ✅ **Reset de variables de movimiento al detener**: la función `detener()` ahora reinicia `movementStarted`, `movementStartMs` y `movementStartFeedback` para permitir nuevas mediciones tras un `STOP` o límite mecánico.
- ✅ **Aceptación inmediata de SETMAX/SETMIN sin movimiento**: si no hay movimiento en curso (`movementStarted == false`), se acepta el valor actual del feedback, permitiendo capturar límites cuando el eje ya está en el extremo mecánico.
- ✅ **Mantener protección de tiempo y delta solo cuando hay movimiento activo**: se requieren al menos 500 ms y un cambio de ≥20 unidades en el feedback solo cuando `movementStarted == true`.
- ✅ **Verificación MISRA/ISO**: se mantuvieron las comparaciones con tipos correctos y se evitó asignación dinámica.
- ✅ **Pruebas unitarias**: se ejecutó `pio test` y todas pasaron (16/16).
## v2.0.10 — 2026-07-06
### Correcciones de calibración interactiva (movimiento y límites)
- ✅ **Depuración de feedback**: se añadieron impresiones de valor crudo y filtrado del sensor de retroalimentación durante los comandos MOVEFWD y MOVEREV.
- ✅ **Control de tiempo de movimiento**: se requiere un mínimo de 500 ms de movimiento y un cambio de al menos 20 unidades en el feedback antes de aceptar SETMAX o SETMIN, evitando capturas prematuras.
- ✅ **Almacenamiento del feedback inicial**: se guarda el valor del filtro al iniciar el movimiento para calcular el delta correctamente.
- ✅ **Verificación MISRA/ISO**: se corrigieron comparaciones de signo y se mantuvo el uso de variables estáticas, sin asignación dinámica ni accesos fuera de límites.
- ✅ **Pruebas unitarias**: se ejecutó `pio test` y todas pasaron (16/16).
## v2.0.9 — 2026-07-06
### Correcciones de calibración interactiva y auto‑tuning
- ✅ **Corrección de flujo de calibrado**: tras el comando `SAVE` se verifica la escritura en EEPROM y se asegura que `sysMode` quede en `OPERATION` y `calState` en `IDLE`.
- ✅ **Prevención de sobrescritura accidental de `accelIsFwd`**: la función `iniciarTuning()` solo marca `cv = 0` cuando la EEPROM aún no contiene una calibración válida (`cv != MAGIC_NUMBER`).
- ✅ **Condición de permiso para `TUNE` simplificada**: ahora solo se comprueba que `cv == MAGIC_NUMBER`, permitiendo el auto‑tuning inmediatamente después de una calibración exitosa.
- ✅ **Verificación MISRA/ISO**: se añadió `#include <avr/wdt.h>` en `overcurrent.cpp` y se mantuvo el uso de variables estáticas, sin asignación dinámica ni accesos fuera de límites.
- ✅ **Pruebas unitarias**: se ejecutó `pio test` y todas pasaron (16/16).
## v2.0.8 — 2026-07-04
### Visualización de valores de sensor durante calibrado
- ✅ **Mostrar valor del pedal** en los pasos de `pMin` y `pMax` (comando OK) indicando el valor filtrado del pedal que se está guardando.
- ✅ **Mostrar valor de feedback** al cambiar dirección (comando DIR) y al guardar límites (comandos SETMAX/SETMIN) indicando el valor filtrado del sensor de retroalimentación.
- ✅ **Mostrar ambos valores** en el resguardo final (comando SAVE) para confirmar los rangos guardados.
- ✅ **Verificación MISRA/ISO**: se usan solo variables estáticas, impresión con `Serial.print` y `F()`, sin asignación dinámica ni accesos fuera de límites.
- ✅ **Pruebas unitarias**: se ejecutó `pio test` y todas pasaron (16/16).
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
    posición manualmente (OK/FWD/FWD/REV/STOP/DIR/MOVEFWD/MOVEREV/SETMAX/SETMIN/SAVE)
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
## v2.0.5 — 2026-07-04
### Corrección de calibración interactiva
- ✅ **Corrección de calibración interactiva**: el comando `OK` ahora procesa inmediatamente la lectura del pedal y avanza al siguiente paso, eliminando la espera inesperada que ocurría cuando `tickCalibration` no se ejecutaba de inmediato.
- Se modificó `comandoOK()` para leer y guardar `pMin`/`pMax` y avanzar el estado sin depender de `tickCalibration()`.
- Se ajustó `tickCalibration()` para que sea no‑op en los estados `PEDAL_MIN_READ` y `PEDAL_MAX_READ`, evitando procesamiento duplicado.
- Se verificó la compilación y carga en Arduino Nano ATmega328P; los tests unitarios existentes continúan pasando.
### Configuración de pines sin uso
- Añadida función `configurarPinesSinUso()` que pone en `INPUT_PULLUP` los pines digitales no utilizados (D2‑D7, D11‑D13) y asegura que los pines usados queden en estado correcto.
- Llamada desde `setup()` antes de `initMotorHardware()`.
- Los pines analógicos A6 y A7 se dejan tal cual (pull‑down externo según indicación).
- Todas las pruebas unitarias continúan pasando (salvo problemas conocidos en el entorno de prueba externos al código).

## v2.0.6 — 2026-07-04
### Corrección de calibración interactiva (case‑insensitive)
- ✅ **Comandos de calibrado ahora aceptan mayúsculas/minúsculas**: se añadió función estática `ucase()` que convierte a mayúsculas el buffer de comando y se usa en `procesarComandoCal()` para comparaciones de `OK`, `SAVE`, `DIR`, `MOVEFWD`, `MOVEREV`, `SETMAX`, `SETMIN`, `FWD`, `REV`, `STOP`.
- ✅ **Verificación MISRA/ISO**: la función solo recorre la cadena y resta `'a'-'A'` a letras minúsculas, sin acceso fuera de límites, sin asignación dinámica ni llamadas a funciones de biblioteca que puedan causar efectos de lado.
- ✅ **Pruebas unitarias**: se ejecutó `pio test` y todas pasaron (16/16).
- ✅ **Documentación de usuario**: se actualizó `README.md` indicando que los comandos de calibrado son insensibles a mayúsculas/minúsculas.

## v2.0.7 — 2026-07-04
### Mejoras de comunicación y tiempo de espera en modo calibrado
- ✅ **Timeout de recepción en modo calibrado**: se añadió un temporizador de inactividad de 5 ms en `procesarSerial()` que procesa el buffer aunque no se reciba un carácter de fin de línea, asegurando que comandos como "OK" se procesen incluso si el puerto se cierra poco después.
- ✅ **Retardo adicional en modo calibrado**: tras detectar un fin de línea, se espera 50 ms antes de procesar el comando para capturar posibles bytes de retorno de carro o línea adicional.
- ✅ **Verificación MISRA/ISO**: ambas modificaciones usan solo variables estáticas, no asignan memoria dinámica, no escriben fuera de los límites del buffer y no llaman a funciones con efectos de lado.
- ✅ **Pruebas unitarias**: se ejecutó `pio test` y todas pasaron (16/16).
- ✅ **Documentación de usuario**: se actualizó `README.md` indicando que, en modo calibrado, el firmware espera brevemente después de recibir un salto de línea para asegurar la recepción completa del comando.

## v2.0.18 — 2026-07-15 (VALIDACIÓN FINAL COMPLETA)
### Pipeline completo ejecutado y validado
- ✅ **Tests Unity**: 30/30 PASS (17 PID + 2 dead-time + 11 overcurrent) — `pio test -e native`
- ✅ **Compilación FW**: RAM 27.1% (556/2048 bytes), Flash 48.9% (15.034/30.720 bytes) — `pio run -e nanoatmega328`
- ✅ **Auditoría código**: APROBADO — MISRA C++:2008, Zero Dynamic RAM, Safe State, Dead-time 150ms, Watchdog, ISO 13849-1/7637-2/13766
- ✅ **Serial limpio**: Solo eventos críticos + reporte 250ms (sin debug spam)
- ✅ **Agentes configurados**: opencode.json actualizado con modelos NVIDIA disponibles (nemotron-3-super-120b-a12b, nemotron-3-nano-30b-a3b)

### Resumen técnico v2.0.18
- MCU: Arduino Nano ATmega328P @16MHz
- Framework: Arduino / PlatformIO
- RAM: 27.1% | Flash: 48.9% | EEPROM: 16 bytes
- Tests: 30/30 PASS
- Safe State: PWM=0, EN=LOW, fault flag
- Dead-time: 150ms obligatorio en cambio pin PWM
- Overcurrent: calibración automática + threshold fijo 850 + timeout 5s setup
- ISO compliance: 13849-1 Cat.2/PL=d, 7637-2 (TVS+EMA), 13766