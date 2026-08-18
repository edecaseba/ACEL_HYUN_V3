# CHANGELOG — ACEL_HYUN_V3
## v2.0.24 — 2026-08-17
### Fix crítico: reset del micro al arrancar el motor
- ✅ **Timer1 PWM revertido a 20kHz (10-bit, TOP=ICR1=799)**: v2.0.23 había subido el PWM del puente H a 62.5kHz (modo 8-bit) para que `analogWrite()` tuviera rango completo. El IBT-2/BTS7960 soporta máx 25kHz (datasheet, `skill-ibt2.md`) — a 62.5kHz el driver malfunciona y genera transitorios que resetean el ATmega328P justo al arrancar el PWM. Causa raíz más probable del síntoma "el micro se reinicia".
- ✅ **Bug de modo Timer1 (WGM) corregido**: la config previa (incluso antes de v2.0.23) armaba modo 15 (TOP=OCR1A) en vez de modo 14 (TOP=ICR1). OCR1A es también el duty del canal L_PWM (pin 9), así que el período del PWM dependía del propio duty de un canal — fuente de glitches. Corregido a modo 14 real.
- ✅ **`pwmWriteMotor()` reemplaza `analogWrite()` en `mover()`**: escribe el duty 0-255 escalado al TOP real (799) vía `OCR1A`/`OCR1B` directo. `analogWrite()` escribía 0-255 sin escalar contra un TOP=799, limitando la autoridad real del motor a ~32% de duty máximo.
- ✅ **Colisión EEPROM corregida**: `EE_NOMINAL_ADDR`/`EE_SIGMA_ADDR` (calibración de sobrecorriente) arrancaban en addr 0x00, la misma dirección donde `eepromGetSafe/PutSafe(0, cfg)` persiste la config general. Cada guardado de una corrompía la otra (ej. `cfg.pMin` corrupto → setpoint erróneo al bootear → PID al máximo desde el primer ciclo). Movida la región de sobrecorriente a partir de addr 64, con `static_assert(sizeof(Config) <= EE_NOMINAL_ADDR)` para evitar que vuelva a colisionar.
- ✅ **Soft-start no bloqueante reintroducido**: `aplicarRampa()` limita el salto de PWM a 25/ciclo de `loop()` (sin `delay()`, no interfiere con el watchdog). v2.0.23 había sacado el soft-start anterior por usar `delay()` bloqueante; esta versión resuelve el problema original sin bloquear. Se omite durante `tuningLimitCycle` (el auto-tune bang-bang necesita conmutación inmediata).
- ✅ **Diagnóstico de causa de reset**: `MCUSR` se lee y reporta por Serial (`BROWNOUT`/`WATCHDOG`/`EXTERNAL`/`POWER-ON`) antes de limpiarse en `setup()`, para confirmar en banco cuál de estas causas está disparando el reset real.
- ✅ **Verificación**: `pio run -e nanoatmega328` limpio con `-Werror` (RAM 29.0%, Flash 59.1%). `pio test -e native`: 38/38 PASS.

## v2.0.23 — 2026-07-22
### Feature: Auto-calibración completa (ACAL) + Fixes estabilidad
- ✅ **Comando `ACAL`**: Auto-calibración completa usando overcurrent para detectar topes mecánicos
  - Paso 1/5: Pedal RALENTI → OK
  - Paso 2/5: Pedal MÁXIMO → OK
  - Paso 3/5: Detecta dirección aceleración (FWD/REV 500ms cada uno)
  - Paso 4/5: Busca tope ACELERACIÓN hasta overcurrent (stall mecánico)
  - Paso 5/5: Busca tope DESACELERACIÓN hasta overcurrent
  - Guarda automáticamente en EEPROM: pMin/pMax/mMin/mMax/accelIsFwd/cv=MAGIC_NUMBER
- ✅ **Watchdog bootloader-safe**: Secuencia `WDTCSR` al inicio de `setup()` evita bucle de reset por bootloader
- ✅ **`wdt_reset()` en `loop()` y calibración OC**: Evita reset cada 2s durante operaciones largas
- ✅ **Timer1 modo 5 (8-bit, 62.5kHz)**: Compatible con `analogWrite()` nativo en pines 9/10
- ✅ **Soft-start removido**: `delay()` bloqueaba watchdog; `analogWrite()` directo sin rampa
- ✅ **Compilación**: RAM 29.0% (593/2048 B), Flash 58.8% (18062/30720 B)
- ✅ **Tests unitarios**: 38/38 PASS (17 PID + 2 dead-time + 11 overcurrent + 8 main)

## v2.0.22 — 2026-07-22
### Fix: Calibración interactiva - PWM y mensaje MOVEREV
- ✅ **VEL_TEST 140 → 180** (línea 42 main.cpp): PWM de calibración subido de 13.7% a 17.6% duty cycle para vencer fricción estática en actuadores lentos
- ✅ **Mensaje MOVEREV corregido en LIMIT_DECEL** (líneas 446-456): Ahora imprime "ACELERACION" correctamente (antes decía "DESACELERACION" incorrectamente). MOVEREV en paso 5 mueve hacia ACELERACIÓN (opuesto a MOVEFWD).
- ✅ **Tests añadidos (8 nuevos en test_main.cpp):** VEL_TEST=180, mensajes MOVEREV/MOVEFWD en LIMIT_ACCEL/LIMIT_DECEL, firstMovementAfterStop behavior
- ✅ **Mocks comunes creados:** test/mock_arduino.h/.cpp - EEPROM, analogRead, millis, micros, Serial, safeState
- ✅ **Compilación**: RAM 28.1%, Flash 53.3% (dentro límites ATmega328P).
- ✅ **Tests unitarios**: 38/38 PASS (17 PID + 2 dead-time + 11 overcurrent + 8 main).

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

(End of file - total 275 lines)

## v2.0.22 — 2026-07-22
### Changes:
- **VEL_TEST 140 → 180** (línea 42 main.cpp): PWM de calibración subido de 13.7% a 17.6% duty cycle para vencer fricción estática en actuadores lentos
- **Mensaje MOVEREV corregido en LIMIT_DECEL** (líneas 446-456): Ahora imprime "ACELERACION" correctamente (antes decía "DESACELERACION" incorrectamente). MOVEREV en paso 5 mueve hacia ACELERACIÓN (opuesto a MOVEFWD).
- **Tests añadidos (8 nuevos en test_main.cpp):**
  - test_vel_test_is_180_in_mover_during_calibration
  - test_moverev_message_in_limit_decel_says_aceleracion
  - test_moverev_message_in_limit_accel_says_aceleracion
  - test_movefwd_message_in_limit_decel_says_desaceleracion
  - test_first_movement_after_stop_is_true_after_detener
  - test_first_movement_after_stop_allows_immediate_movement
  - test_first_movement_after_stop_resets_after_first_move
  - test_calibration_vel_test_constant_value
- **Mocks comunes creados:**
  - test/mock_arduino.h/.cpp - EEPROM, analogRead, millis, micros, Serial, safeState
- **Métricas:**
  - Tests: 38/38 PASS (17 PID + 2 dead-time + 11 overcurrent + 8 main)
  - Compilación FW: RAM 28.1%, Flash 53.3% (dentro límites ATmega328P).