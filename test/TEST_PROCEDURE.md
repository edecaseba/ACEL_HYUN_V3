# Procedimiento de Prueba — Controlador de Acelerador (ACEL_HYUN_V3)
**Firmware:** 2.0.0 · **MCU:** ATmega328P (Arduino Nano) · **Fecha:** 2026-06-24

Este documento detalla el procedimiento de prueba en hardware para validar el firmware del
controlador de acelerador, incluyendo la nueva calibración interactiva por comando serial,
auto-tuning PID por relay, y control con asentamiento (anti-ronroneo).

---

## Precauciones de Seguridad
- **Mecánica:** Asegurar que el actuador del acelerador esté desacoplado de la bomba de
  inyección del motor diésel durante las pruebas iniciales.
- **Eléctrica:** Utilizar una fuente de alimentación de 24Vcc con limitación de corriente
  ajustable (setear límite inicial en 2.0A).
- **Hardware:** Verificar resistencias de pull-down externas de 10kΩ en pines analógicos.
- **Parada de Emergencia:** Tener un interruptor físico de corte de energía de 24Vcc al alcance.

---

## Fase 1: Calibración Interactiva (CAL)

Al primer encendido (EEPROM vacía) o al enviar `CAL`, el sistema entra en modo calibración
interactiva. El técnico sigue las instrucciones por serial y confirma cada paso.

### Paso 1 — Pedal mínimo (ralentí)
```
SERIAL TX: "Paso 1/6: Coloque el acelerador en RALENTI (mínimo) y envíe OK"
TÉCNICO:  Posiciona físicamente el pedal en ralentí y envía → OK
SISTEMA:  Lee A0 filtrado, guarda cfg.pMin
          "pMin = XXX guardado."
```

### Paso 2 — Pedal máximo (full RPM)
```
SERIAL TX: "Paso 2/6: Coloque el acelerador a MAXIMAS RPM y envíe OK"
TÉCNICO:  Pisa el pedal a fondo y envía → OK
SISTEMA:  Lee A0 filtrado, guarda cfg.pMax
          "pMax = XXX guardado."
```

### Paso 3 — Test de dirección del motor (IBT-2)
```
SERIAL TX: "Paso 3/6: Use FWD/REV/STOP para probar dirección del motor.
            Luego envíe: DIR FWD ACEL  o  DIR REV ACEL"
TÉCNICO:  Envía → FWD  (el motor se mueve)
          Envía → REV  (el motor se mueve al revés)
          Envía → STOP (detiene)
          Determina qué dirección ACELERA (aumenta RPM/posición)
          Envía → DIR FWD ACEL   (si FWD acelera)
               o → DIR REV ACEL   (si REV acelera)
SISTEMA:  Guarda cfg.accelIsFwd
          "Dirección: R_PWM ACELERA. Guardado."
```

### Paso 4 — Límite máximo del actuador
```
SERIAL TX: "Paso 4/6: Use MOVEFWD para ir al tope de aceleración.
            Cuando llegue, envíe SETMAX"
TÉCNICO:  Envía → MOVEFWD (motor avanza hacia aceleración)
          Observa el actuador hasta que llegue al tope físico
          Envía → SETMAX
SISTEMA:  Guarda cfg.mMax = feedback actual, detiene motor
          "mMax = XXX guardado."
```

### Paso 5 — Límite mínimo del actuador
```
SERIAL TX: "Paso 5/6: Use MOVEREV para ir al tope de desaceleración.
            Cuando llegue, envíe SETMIN"
TÉCNICO:  Envía → MOVEREV (motor retrocede)
          Envía → SETMIN al llegar al tope opuesto
SISTEMA:  Guarda cfg.mMin, detiene motor
          "mMin = XXX guardado."
```

### Paso 6 — Guardar o tunear
```
SERIAL TX: "Paso 6/6: Envíe SAVE para guardar en EEPROM
            o TUNE para auto-ajustar PID primero."
TÉCNICO:  Envía → SAVE  (guarda límites, KP/KI/KD por defecto)
               o → TUNE  (ejecuta auto-tuning antes de guardar)
```

---

## Fase 2: Auto-tuning PID (TUNE)

En cualquier momento post-calibración, el técnico puede enviar `TUNE` para ejecutar
relay auto-tuning (Åström-Hägglund). El sistema:

1. Mueve el actuador a la posición media (~50%)
2. Oscila el relay alrededor del setpoint con histéresis de ±5%
3. Mide el período último (Tu) y la amplitud de oscilación (a)
4. Calcula Ku = 4h / (πa)
5. Aplica Ziegler-Nichols:
   - Kp = 0.6 · Ku
   - Ki = 1.2 · Ku / Tu
   - Kd = 0.075 · Ku · Tu
6. Guarda KP/KI/KD en EEPROM

```
=== AUTO-TUNING PID (RELAY) ===
Moviendo a posición media...
TUNE: Relay iniciado. Observe oscilación...
..... (puntos = ciclos detectados)
TUNE: Ciclos suficientes.

=== AUTO-TUNING COMPLETO ===
Tu = 0.350 s
Ku = 12.45
Kp = 7.47
Ki = 0.0534
Kd = 0.33
Valores guardados en EEPROM.
```

---

## Fase 3: Operación Normal (Control con Asentamiento)

Post-calibración, el sistema controla el motor continuamente con:

- **Zona muerta:** |error| ≤ 3 → no mueve motor
- **Asentamiento:** |error| ≤ 3 por 100ms → apaga motor, resetea integral, marca `asentado`
- **Reactivación:** |error| > 6 → despierta PID, reanuda control
- **Reporte serial** cada 250ms: SetP / Act / Err / Kp / Ki / Kd

El motor permanece **completamente apagado (PWM=0)** mientras el pedal esté quieto.
Solo se energiza cuando el operador mueve el pedal más allá del umbral de reactivación.

---

## Comandos Seriales — Referencia Rápida

| Comando | Modo | Función |
|---------|------|---------|
| `CAL` | Cualquiera | Inicia calibración interactiva |
| `OK` | CAL (pasos 1-2) | Confirma posición del pedal |
| `FWD` | CAL (paso 3) | Mueve motor adelante (test dirección) |
| `REV` | CAL (paso 3) | Mueve motor atrás |
| `STOP` | CAL / TUNE | Detiene motor inmediatamente |
| `DIR FWD ACEL` | CAL (paso 3) | FWD = acelerar |
| `DIR REV ACEL` | CAL (paso 3) | REV = acelerar |
| `MOVEFWD` | CAL (pasos 4-5) | Mueve hacia aceleración |
| `MOVEREV` | CAL (pasos 4-5) | Mueve hacia desaceleración |
| `SETMAX` | CAL (paso 4) | Captura feedback como límite superior |
| `SETMIN` | CAL (paso 5) | Captura feedback como límite inferior |
| `SAVE` | CAL (paso 6) | Guarda configuración en EEPROM |
| `TUNE` | Post-CAL | Ejecuta relay auto-tuning PID |
| `RST` | Cualquiera | Resetea condición de falla (stall) |

---

## Procedimiento Completo de Puesta en Marcha

1. **Conexión:** Alimentar el sistema 24V, conectar USB-Serial a 115200 baud
2. **Primer encendido:** El sistema inicia `CAL` automáticamente si EEPROM está virgen
3. **Seguir pasos 1-6** de la calibración interactiva
4. **Opcional:** Enviar `TUNE` para auto-ajustar PID
5. **Operación:** El PID controla el motor continuamente con asentamiento
6. **Si falla (stall):** Enviar `RST` para despejar fault

---

## Casos de Prueba en Hardware

### Caso 1: Calibración de pedal mínimo
- Enviar `CAL`, seguir instrucciones hasta paso 1
- Poner pedal en ralentí, enviar `OK`
- **PASA si:** `pMin` se guarda con un valor coherente (~0-200 para pedal suelto)

### Caso 2: Calibración de pedal máximo
- En paso 2, pisar pedal a fondo, enviar `OK`
- **PASA si:** `pMax` > `pMin` + 200

### Caso 3: Test de dirección
- En paso 3, enviar `FWD` y `REV`, observar dirección
- Enviar `DIR FWD ACEL` o `DIR REV ACEL`
- **PASA si:** El motor responde a ambos comandos y la dirección se guarda

### Caso 4: Captura de límites del actuador
- En pasos 4-5, mover a topes y enviar `SETMAX`/`SETMIN`
- **PASA si:** `mMax > mMin` con diferencia > 50

### Caso 5: SAVE en EEPROM
- En paso 6, enviar `SAVE`
- **PASA si:** Mensaje "CALIBRACION GUARDADA" y al reiniciar no entra en CAL

### Caso 6: Auto-tuning TUNE
- Post-calibración, enviar `TUNE`
- **PASA si:** El relay oscila ~5 ciclos, calcula valores y los guarda

### Caso 7: Asentamiento
- Con pedal quieto, observar reporte serial
- **PASA si:** El motor está completamente apagado y aparece `[ASENTADO]`

### Caso 8: Reactivación
- Mover el pedal rápidamente
- **PASA si:** El PID se reactiva, el motor se mueve suavemente, sin ronroneo

### Caso 9: Stall
- Bloquear el actuador con el motor en movimiento
- **PASA si:** `monitorStallCurrent()` detecta sobrecorriente, bloquea EN, mensaje crítico

### Caso 10: RST post-falla
- Enviar `RST` tras un stall
- **PASA si:** Fault se limpia, motor vuelve a responder

### Caso 11: Dead-time (anti shoot-through 150ms)
- **Objetivo:** Verificar que el dead-time de 150ms prevenga cambios de dirección inmediatos del motor, evitando cortocircuitos entre transistores (shoot-through).
- **Procedimiento:**
  1. Completar calibración (CAL) y guardar (SAVE) para tener un sistema operativo estable.
  2. Enviar `STOP` para detener el motor.
  3. Enviar `MOVEFWD` para mover el actuador hacia adelante.
  4. Inmediatamente después, enviar `MOVEREV` para cambiar dirección.
  5. **Verificación:** El motor debe continuar moviéndose hacia adelante por al menos 150ms antes de responder a MOVEREV.
  6. Esperar 150ms, luego enviar `MOVEREV` nuevamente.
  7. **PASA si:** El motor ahora se mueve hacia atrás, confirmando que el dead-time se completó.
- **PASA si:** El motor mantiene la dirección anterior por ≥150ms después del cambio de dirección solicitado, confirmando el dead-time anti shoot-through de 150ms.

### Caso 12: Validación Overcurrent y STALL
- **Objetivo:** Verificar que la detección de sobrecorriente (threshold 850) y stall (threshold 950) funcionen correctamente sin falsos positivos en operación normal.
- **Procedimiento:**
  1. Completar calibración (CAL) y guardar (SAVE).
  2. Enviar `OCAL` → verifica: `[OC] Calibrated nominal=XXX sigma=Y` (nominal ~210).
  3. Enviar `RST` para limpiar cualquier fault previo.
  4. Enviar `FWD` → motor se mueve, **NO** debe aparecer `[OC] Over-current!` ni `[STALL]`.
  6. Enviar `REV` → motor cambia dirección tras dead-time 150ms, **NO** debe disparar fault.
  7. Enviar `STOP` → motor se detiene.
  7. (Opcional) Bloquear actuador mecánicamente → debe aparecer `[STALL] raw=XXX filt=YYY` y `CRITICAL: STALL DETECTADO`.
  8. Enviar `RST` → sistema recuperado.
- **PASA si:** 
  - Motor opera sin falsos positivos de overcurrent/stall en trabajo normal (~700-800 ADC).
  - `OCAL` calibra correctamente (nominal ~210, sigma ~19).
  - `OCAL` con motor moviéndose da error: `[OC] Error: Motor en movimiento. Detenga antes de calibrar.`
  - Comando `STOP ` (con espacio) reconocido como `STOP`.
  - Stall real (>950) detecta y bloquea EN.

### Caso 13: Verificación PWM en pines 9/10 (Fix v2.0.15)
- **Objetivo:** Confirmar que la inicialización del hardware preserva la configuración PWM del Arduino core en pines 9 (OC1A) y 10 (OC1B).
- **Procedimiento:**
  1. Encender el sistema (o enviar `RST` si ya está calibrado).
  2. Enviar `FWD` → verificar que el motor gira hacia adelante (R_PWM activo en pin 10).
  3. Enviar `STOP` → motor se detiene.
  4. Enviar `REV` → verificar que el motor gira hacia atrás (L_PWM activo en pin 9).
  5. Opcional: medir con osciloscopio/multímetro PWM en pines 9 y 10 durante FWD/REV.
- **PASA si:** El motor responde a ambos comandos FWD y REV con movimiento real (no solo ruido audible), confirmando que la salida PWM funciona en ambos pines.

### Caso 14: Validación calibración overcurrent robusta (Fix v2.0.16)
- **Objetivo:** Verificar que la calibración de overcurrent rechace lecturas inválidas (sensor desconectado) y que el setup tenga timeout.
- **Procedimiento:**
  1. Desconectar sensor de corriente del pin A2 (o dejar flotante).
  2. Resetear el sistema (power cycle o botón reset).
  3. **Verificar:** Tras ~5s aparece `[OC] Timeout calibracion. Sensor A2 desconectado?` y el sistema continúa.
  4. Reconectar sensor A2 correctamente.
  5. Enviar `OCAL` → verifica: `[OC] Calibrated nominal=XXX sigma=Y` (nominal ~210).
  6. Enviar `RST` para limpiar cualquier fault.
  7. Enviar `FWD` → motor se mueve sin `[OC] Over-current!`.
- **PASA si:**
  - Setup no se bloquea indefinido con A2 flotante (timeout 5s).
  - `OCAL` con sensor desconectado da error: `[OC] Error: Lectura invalida (nominal=0 o saturado). Verifique sensor A2.`
  - `OCAL` con sensor conectado calibra correctamente (nominal ~210, sigma ~19).
  - Sistema opera sin falsos positivos overcurrent.

### Caso 15: Dead-time anti-plugging reforzado (Fix v2.0.17)
- **Objetivo:** Verificar que el dead-time de 150ms se active SIEMPRE que cambie el pin PWM activo (R_PWM ↔ L_PWM), previniendo inversión brusca bajo carga (plugging) que cause overcurrent real.
- **Procedimiento:**
  1. Completar calibración (CAL) y guardar (SAVE).
  2. Enviar `FWD` → motor acelera.
  3. Inmediatamente enviar `REV` (cambio brusco de dirección).
  4. **Verificar:** Motor continúa en FWD por ≥150ms antes de responder a REV.
  5. Esperar 200ms, enviar `REV` nuevamente.
  6. **Verificar:** Motor ahora gira en REV.
  7. Repetir con `MOVEFWD` → `MOVEREV` en modo calibración (pasos 4-5).
- **PASA si:**
  - Dead-time se respeta en **cualquier** cambio de pin PWM (no solo por dirección lógica).
  - No aparece `[OC] Over-current! raw=1023` por inversión instantánea.
  - Motor mantiene dirección anterior ≥150ms tras solicitud de cambio.

### Caso 16: Serial no saturado por prints de debug (Fix v2.0.18)
- **Objetivo:** Verificar que el puerto serie no se sature con mensajes de debug, permitiendo recepción de comandos y reportes PID.
- **Procedimiento:**
  1. Sistema en operación normal (post-CAL, SAVE).
  2. Mover pedal rápidamente durante 10s.
  3. Enviar comando `CAL` → debe entrar en modo calibración.
  4. Enviar `RST` → debe resetear fault.
  5. Verificar reportes PID cada 250ms: `SetP:XX Act:XX Err:XX Kp:X.X Ki:X.XX Kd:X.XX`
- **PASA si:**
  - Comandos `CAL`, `RST`, `TUNE`, `OCAL` se procesan inmediatamente.
  - Reportes PID aparecen cada 250ms sin interrupciones.
  - No hay mensajes `MOVER: vel=...` ni `[STALL] raw=...` en operación normal.
