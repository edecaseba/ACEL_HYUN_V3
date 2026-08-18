# Guía de Calibración - ACEL_HYUN_V3

Controlador de acelerador para excavadora Hyundai R250 LC7

---

## 1. Requisitos Previos

### Hardware
- Arduino Nano (ATmega328P) con firmware ACEL_HYUN_V3 cargado
- Puente H IBT-2 (BTS7960) conectado según pinout
- Potenciómetro pedal (A0) y potenciómetro feedback (A1)
- Sensor corriente ACS712/shunt (A2)
- Fuente 24Vcc + step-down XL4005 (5V para Arduino)
- Cable USB para conexión PC-Arduino

### Software
- **Arduino IDE 2.x** (recomendado) o 1.8.x
- Puerto serie disponible (COMx en Windows, /dev/ttyUSBx en Linux)

---

## 2. Conexión y Configuración Arduino IDE

### 2.1 Conectar Arduino
1. Conectar Arduino Nano por USB al PC
2. Verificar que el driver CH340/FTDI esté instalado

### 2.2 Configurar Arduino IDE
```
Herramientas → Placa → Arduino AVR Boards → Arduino Nano
Herramientas → Procesador → ATmega328P (Old Bootloader)  // o ATmega328P según tu placa
Herramientas → Puerto → COMx (Windows) / /dev/ttyUSB0 (Linux)
Herramientas → Velocidad → 115200  // IMPORTANTE: debe coincidir con firmware
```

### 2.3 Abrir Monitor Serie
- Botón **Monitor Serie** (lupa arriba a la derecha) o `Ctrl+Shift+M`
- Configurar en la barra inferior del monitor:
  - **Baud rate: 115200**
  - **Fin de línea: "Nueva línea" (NL)** o "Ambos (NL & CR)"
  - **Codificación: UTF-8**

Al bootear, el firmware imprime la causa del reset anterior (`Reset cause (MCUSR): 0x.. ...`). En un arranque normal por primera vez debería decir `POWER-ON`; si dice `BROWNOUT` o `WATCHDOG` de forma repetida, hay un problema de alimentación o de firmware que hay que resolver antes de calibrar (ver sección 9).

---

## 3. Calibración Manual (paso a paso, control total)

Elegí este camino si preferís posicionar el actuador vos mismo en cada tope, o si `ACAL` (sección 4) falla por algún motivo.

### Paso 0: Verificar Comunicación
Al abrir el monitor serie deberías ver:
```
Sistema listo. Comandos: CAL RST TUNE
```
Si ves caracteres extraños → verificar baud rate 115200.

---

### Paso 1: Iniciar Calibración
Enviar comando:
```
CAL
```
Respuesta esperada:
```
=== CALIBRACION INTERACTIVA ===
Paso 1/6: Coloque el acelerador en RALENTI (minimo) y envie OK
```

---

### Paso 2: Calibrar Pedal Mínimo (Ralentí)
1. **Mover pedal físicamente a posición mínima (ralentí)**
2. Esperar 1-2 segundos a que la lectura se estabilice
3. Enviar:
```
OK
```
Respuesta:
```
pMin = XXX guardado.
Paso 2/6: Coloque el acelerador a MAXIMAS RPM y envie OK
```

---

### Paso 3: Calibrar Pedal Máximo
1. **Mover pedal físicamente a posición máxima (full RPM)**
2. Esperar 1-2 segundos
3. Enviar:
```
OK
```
Respuesta:
```
pMax = XXX guardado.
Paso 3/6: Use FWD/REV/STOP para probar direccion del motor.
Luego envie: DIR FWD ACEL  o  DIR REV ACEL
```

---

### Paso 4: Probar Dirección del Motor
**Objetivo:** Saber qué lado del puente H acelera el motor.

#### 4.1 Probar FWD
Enviar:
```
FWD
```
El motor gira. Observar: **¿Acelera o desacelera el actuador?**

#### 4.2 Probar REV
Enviar:
```
STOP
REV
```
El motor gira en sentido opuesto. Observar: **¿Acelera o desacelera?**

#### 4.3 Detener
```
STOP
```

---

### Paso 5: Configurar Dirección de Aceleración

**Si FWD acelera (R_PWM = acelera):**
```
DIR FWD ACEL
```
Respuesta:
```
Direccion: R_PWM ACELERA. Guardado.
Paso 4/6: Use MOVEFWD para ir al tope de ACELERACION.
Cuando llegue, envie SETMAX
```

**Si REV acelera (L_PWM = acelera):**
```
DIR REV ACEL
```
Respuesta:
```
Direccion: L_PWM ACELERA. Guardado.
Paso 4/6: Use MOVEREV para ir al tope de ACELERACION.
Cuando llegue, envie SETMAX
```

> **IMPORTANTE:** El firmware te dice exactamente qué comando usar según tu configuración.

---

### Paso 6: Capturar Tope de ACELERACIÓN (SETMAX)

**Si configuraste `DIR FWD ACEL`:**
```
MOVEFWD
```
El motor mueve hacia aceleración. Cuando llegue al tope mecánico:
```
STOP
SETMAX
```

**Si configuraste `DIR REV ACEL`:**
```
MOVEREV
```
El motor mueve hacia aceleración. Cuando llegue al tope:
```
STOP
SETMAX
```

Respuesta:
```
mMax = XXX guardado (tope ACELERACION).
Paso 5/6: Use MOVEREV para ir al tope de DESACELERACION.
Cuando llegue, envie SETMIN
```
(o `MOVEFWD` según tu configuración)

---

### Paso 7: Capturar Tope de DESACELERACIÓN (SETMIN)

Usa el comando opuesto al paso anterior:

**Si usaste MOVEFWD para SETMAX:**
```
MOVEREV
```
**Si usaste MOVEREV para SETMAX:**
```
MOVEFWD
```

Cuando llegue al tope opuesto:
```
STOP
SETMIN
```

Respuesta:
```
mMin = XXX guardado (tope DESACELERACION).
Paso 6/6: Envie SAVE para guardar en EEPROM
         o TUNE para auto-ajustar PID primero.
```

---

### Paso 8: Guardar Calibración Básica
```
SAVE
```
Respuesta:
```
=== CALIBRACION GUARDADA EN EEPROM ===
Envie TUNE para auto-ajustar PID, o cambie a operacion normal.
```

**La calibración básica está completa.** El sistema ya puede operar con PID por defecto (Kp=2.0, Ki=0.1, Kd=0.5).

---

## 4. Auto-Calibración (ACAL) — alternativa rápida, sin buscar los topes a mano

`ACAL` calibra el pedal igual que el modo manual (2 pasos con `OK`), pero después busca los topes mecánicos del actuador **sola**, moviendo el motor hasta que el sensor de sobrecorriente detecta que llegó al tope físico (stall). No hace falta usar `FWD`/`REV`/`MOVEFWD`/`MOVEREV`/`SETMAX`/`SETMIN` a mano.

> **Requisito:** el sensor de corriente debe estar calibrado antes de correr `ACAL` (`oc_isCalibrated()`). El firmware lo calibra solo al bootear si detecta que no lo está; si tenés dudas, corré `OCAL` primero con el motor parado. Si el sensor A2 no está calibrado, `ACAL` no va a detectar los topes y va a hacer timeout a los 30s en cada paso de búsqueda.

### 4.1 Iniciar
```
ACAL
```
Respuesta:
```
Iniciando auto-calibracion completa...
=== AUTO-CALIBRACION COMPLETA ===
Paso 1/5: Coloque pedal en RALENTI y envie OK
```

### 4.2 Pedal mínimo y máximo (igual que manual)
1. Pedal en ralentí → `OK` → guarda `pMin`, pasa a "Paso 2/5: Coloque pedal a MAXIMAS RPM y envie OK"
2. Pedal a fondo → `OK` → guarda `pMax`, pasa a "Paso 3/5: Detectando direccion de aceleracion..."

### 4.3 El resto es automático
- **Paso 3/5:** el firmware prueba FWD 500ms y REV 500ms por su cuenta y decide cuál dirección acelera — no hay que hacer nada.
- **Paso 4/5:** mueve el actuador hacia aceleración hasta que el sensor de corriente detecta el tope mecánico (overcurrent = stall) y guarda `mMax`.
- **Paso 5/5:** mueve hacia desaceleración hasta el tope opuesto y guarda `mMin`.

Cada paso 4/5 tiene un timeout de seguridad de 30s: si no encuentra el tope en ese tiempo, aborta con un mensaje de error y hay que terminar la calibración a mano (`CAL`, sección 3) o revisar el sensor de corriente (`OCAL`).

### 4.4 Guardado automático
Si `mMax`/`mMin` y `pMax`/`pMin` tienen un recorrido razonable (≥50 en ambos), guarda solo en EEPROM:
```
=== AUTO-CALIBRACION GUARDADA EN EEPROM ===
Envie TUNE para auto-ajustar PID, o RST para operacion normal.
```
Si el recorrido detectado es demasiado chico (<50), imprime el error correspondiente y no guarda — revisar mecánicamente el actuador antes de reintentar.

> ⚠️ **Durante los pasos 3/5-5/5 el motor se mueve solo, sin que vos lo controles paso a paso.** Valen las mismas precauciones de seguridad que en cualquier movimiento del actuador (sección 9): área libre y acceso a corte de emergencia antes de enviar `ACAL`.

---

## 5. Auto-Tuning PID (Opcional pero Recomendado)

El auto-tuning calcula los parámetros PID óptimos para tu actuador específico. Sirve tanto después de una calibración manual (sección 3) como de una `ACAL` (sección 4).

### 5.1 Ejecutar Auto-Tuning
```
TUNE
```

### 5.2 Qué Esperar
```
=== AUTO-TUNING PID (LIMIT CYCLE) ===
Rango: mMin=0 mMax=1019
Moviendo a tope ACELERACION (mMax)...
TUNE: En mMax. Iniciando ciclo limite mMax<->mMin...
TUNE: pos=40 dir=->mMax ciclos=0
.TUNE: pos=23 dir=->mMin ciclos=1
.TUNE: pos=99 dir=->mMax ciclos=2
.TUNE: pos=0 dir=->mMin ciclos=3
.
TUNE: Ciclos suficientes.

=== AUTO-TUNING COMPLETO ===
Rango real: 0.0 - 99.0
Amplitud: 99.0%
Tu = 12.345 s
Ku = 45.67
Kp = 27.40
Ki = 4.4444
Kd = 20.00
Valores guardados en EEPROM.
Envie CAL para recalibrar limites, o RST para operacion normal.
```

**El motor oscilará físicamente entre los topes calibrados (mMin ↔ mMax).** Esto es normal y necesario para medir la respuesta del sistema.

### 5.3 Si Fallo la Validación (Amplitud Muy Pequeña)
```
TUNE: Oscilacion insuficiente. PID calculado pero no validado.
Envia SAVEPID para guardar anyway, o CAL para recalibrar.
```
Enviar:
```
SAVEPID
```
Guarda el PID calculado aunque no validó completamente.

---

## 6. Operación Normal

Después de calibrar (y opcionalmente tunear):
```
RST
```
Respuesta:
```
Fallo reseteado. Sistema listo.
```

Ahora mueve el pedal: el actuador seguirá la posición del pedal con control PID.

**Monitor serie muestra cada 250ms:**
```
SetP:45 Act:44 Err:1 Kp:27.4 Ki:4.44 Kd:20.00
SetP:45 Act:45 Err:0 [ASENTADO] Kp:27.4 Ki:4.44 Kd:20.00
```
- `SetP`: Setpoint del pedal (0-100%)
- `Act`: Posición real actuador (0-100%)
- `Err`: Error (SetP - Act)
- `[ASENTADO]`: Motor detenido por error pequeño (anti-ronroneo)

---

## 7. Comandos de Referencia Rápida

| Comando | Descripción | Cuándo Usar |
|---------|-------------|-------------|
| `CAL` | Iniciar calibración manual interactiva | Primera vez / recalibrar con control total |
| `ACAL` | Iniciar auto-calibración (pedal manual + topes automáticos por overcurrent) | Alternativa rápida a `CAL` |
| `OK` | Confirmar posición actual | Pasos 1, 2 de `CAL` o `ACAL` |
| `FWD` / `REV` / `STOP` | Probar dirección motor | Paso 3 de `CAL` |
| `DIR FWD ACEL` | R_PWM acelera | Paso 4 de `CAL` (si FWD acelera) |
| `DIR REV ACEL` | L_PWM acelera | Paso 4 de `CAL` (si REV acelera) |
| `MOVEFWD` / `MOVEREV` | Mover a tope | Pasos 4, 5 de `CAL` |
| `SETMAX` | Guardar tope aceleración | Paso 4 de `CAL` |
| `SETMIN` | Guardar tope desaceleración | Paso 5 de `CAL` |
| `SAVE` | Guardar calibración en EEPROM | Paso 6 de `CAL` (`ACAL` guarda solo al terminar) |
| `TUNE` | Auto-tuning PID | Opcional (recomendado), después de `CAL` o `ACAL` |
| `SAVEPID` | Guardar PID sin validar | Si `TUNE` falla validación |
| `RST` | Reset fault / iniciar operación | Después de calibrar/tunear |
| `OCAL` | Recalibrar sensor corriente | Si cambia hardware A2, o antes de `ACAL` si hay dudas |

> **Nota:** Los comandos **no requieren Enter** (se procesan tras 5ms de inactividad). Funcionan en mayúsculas/minúsculas.

---

## 8. Solución de Problemas

### Motor no se mueve en FWD/REV
- Verificar conexiones IBT-2 (R_PWM=D10, L_PWM=D9, EN=D8)
- Verificar 24V en entrada IBT-2
- Verificar `EN` en HIGH (LED encendido en IBT-2)

### El micro se reinicia al arrancar el motor
- Leer el mensaje `Reset cause (MCUSR)` que imprime el firmware al bootear: si dice `BROWNOUT`, sospechar de la alimentación 5V (XL4005) o de un pico de corriente de arranque; si dice `WATCHDOG`, es un problema de firmware.
- Confirmar con osciloscopio que el PWM en D9/D10 conmuta a ~20kHz (no a otra frecuencia) — el IBT-2/BTS7960 admite máximo 25kHz, por encima de eso el driver malfunciona.
- Ver `.claude/skills/motor-startup-diagnostics/SKILL.md` en el repo para el historial completo de este tipo de fallas ya investigadas.

### "Comando no reconocido"
- Verificar baud rate 115200 en monitor serie
- No escribir espacios extra: `DIR FWD ACEL` ✓ / `DIR FWD ACEL ` ✗

### TUNE no oscila / "Amplitud muy pequena"
- Verificar que SETMAX/SETMIN (o `ACAL`) capturaron topes reales
- Aumentar `TUNE_PWM` en código (actual 180/255) si motor muy débil
- Verificar que no hay fricción excesiva / atascamiento mecánico

### PID por defecto no funciona bien
- Ejecutar `TUNE` obligatorio para actuadores no estándar
- Si TUNE falla: `SAVEPID` → `RST` → probar operación

### Overcurrent / Stall detectado falsamente
- Ejecutar `OCAL` con motor PARADO (`STOP` primero)
- Verificar sensor A2 (ACS712/shunt) y conexiones

### "CRITICAL: PERDIDA DE SEÑAL PEDAL/FEEDBACK" y el sistema entra en falla
- El actuador **no tiene finales de carrera físicos** — el firmware chequea que la lectura de A0 (pedal) o A1 (feedback) esté dentro del rango calibrado (`pMin`/`pMax`, `mMin`/`mMax`) con un margen de tolerancia; si se sale por más de 50ms, corta el motor por seguridad
- Revisar primero conexión física del potenciómetro correspondiente (cable cortado, wiper flojo) — con un divisor resistivo simple, un cable suelto puede dar cualquier lectura, no necesariamente 0 o al fondo de escala
- Si la conexión está bien, puede ser una calibración vieja/corrupta: recalibrar con `CAL` o `ACAL`
- `RST` limpia el fallo una vez resuelta la causa

### ACAL hace timeout buscando el tope (Paso 4/5 o 5/5)
- Confirmar que el sensor de corriente está calibrado (`OCAL` con motor parado)
- Si el actuador es muy débil o hay mucha fricción, puede no llegar a generar suficiente corriente para disparar la detección de stall — usar calibración manual (`CAL`, sección 3) en ese caso

---

## 9. Archivos de Log / Debug

Para reportar problemas, captura la salida completa del monitor serie desde `CAL`/`ACAL` hasta `RST` y adjunta:
- Versión firmware (ver `CHANGELOG.md`)
- Mensaje `Reset cause (MCUSR)` de al bootear
- Valores pMin/pMax/mMin/mMax capturados
- Qué comando `DIR` usaste (si fue calibración manual)
- Salida completa de `TUNE` si se ejecutó

---

## 10. Seguridad

⚠️ **El actuador no tiene finales de carrera físicos.** No hay ningún microswitch ni sensor independiente que corte el motor por sí solo al llegar a un extremo — el límite de posición es enteramente software (rango calibrado del potenciómetro de feedback) respaldado por la detección de stall por sobrecorriente cuando el motor efectivamente choca contra el tope mecánico real de la máquina. Por eso una calibración correcta (sección 3 o 4) y no desconectar/dañar el potenciómetro de feedback son críticos — sin ellos, el único respaldo que queda es el stall.

⚠️ **ANTES DE CALIBRAR (manual o `ACAL`):**
- Asegurar área libre alrededor del actuador/motor
- Tener acceso rápido a botón de parada de emergencia / desconexión 24V
- Con `ACAL`, el motor se mueve automáticamente buscando los topes mecánicos (pasos 3/5 a 5/5) sin intervención paso a paso — las mismas precauciones aplican con más razón
- El sistema tiene Safe State automático (corta PWM + EN=LOW) ante:
  - Overcurrent (sensor A2)
  - Stall (corriente > 950 ADC)
  - Pérdida de señal pedal/feedback (lectura fuera del rango calibrado por >50ms — ver sección 8)

---

**Versión documento:** v2.0.25+ (2026-08-17)
**Firmware compatible:** ACEL_HYUN_V3 v2.0.25+
