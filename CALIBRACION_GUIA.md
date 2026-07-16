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

---

## 3. Secuencia de Calibración Completa

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

## 4. Auto-Tuning PID (Opcional pero Recomendado)

El auto-tuning calcula los parámetros PID óptimos para tu actuador específico.

### 4.1 Ejecutar Auto-Tuning
```
TUNE
```

### 4.2 Qué Esperar
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

### 4.3 Si Fallo la Validación (Amplitud Muy Pequeña)
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

## 5. Operación Normal

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

## 6. Comandos de Referencia Rápida

| Comando | Descripción | Cuándo Usar |
|---------|-------------|-------------|
| `CAL` | Iniciar calibración interactiva | Primera vez / recalibrar |
| `OK` | Confirmar posición actual | Pasos 1, 2 de calibración |
| `FWD` / `REV` / `STOP` | Probar dirección motor | Paso 3 |
| `DIR FWD ACEL` | R_PWM acelera | Paso 4 (si FWD acelera) |
| `DIR REV ACEL` | L_PWM acelera | Paso 4 (si REV acelera) |
| `MOVEFWD` / `MOVEREV` | Mover a tope | Pasos 4, 5 |
| `SETMAX` | Guardar tope aceleración | Paso 4 |
| `SETMIN` | Guardar tope desaceleración | Paso 5 |
| `SAVE` | Guardar calibración en EEPROM | Paso 6 |
| `TUNE` | Auto-tuning PID | Opcional (recomendado) |
| `SAVEPID` | Guardar PID sin validar | Si TUNE falla validación |
| `RST` | Reset fault / iniciar operación | Después de calibrar/tunear |
| `OCAL` | Recalibrar sensor corriente | Si cambia hardware A2 |

> **Nota:** Los comandos **no requieren Enter** (se procesan tras 5ms de inactividad). Funcionan en mayúsculas/minúsculas.

---

## 7. Solución de Problemas

### Motor no se mueve en FWD/REV
- Verificar conexiones IBT-2 (R_PWM=D10, L_PWM=D9, EN=D8)
- Verificar 24V en entrada IBT-2
- Verificar `EN` en HIGH (LED encendido en IBT-2)

### "Comando no reconocido"
- Verificar baud rate 115200 en monitor serie
- No escribir espacios extra: `DIR FWD ACEL` ✓ / `DIR FWD ACEL ` ✗

### TUNE no oscila / "Amplitud muy pequena"
- Verificar que SETMAX/SETMIN capturaron topes reales
- Aumentar `TUNE_PWM` en código (actual 180/255) si motor muy débil
- Verificar que no hay fricción excesiva / atascamiento mecánico

### PID por defecto no funciona bien
- Ejecutar `TUNE` obligatorio para actuadores no estándar
- Si TUNE falla: `SAVEPID` → `RST` → probar operación

### Overcurrent / Stall detectado falsamente
- Ejecutar `OCAL` con motor PARADO (`STOP` primero)
- Verificar sensor A2 (ACS712/shunt) y conexiones

---

## 8. Archivos de Log / Debug

Para reportar problemas, captura la salida completa del monitor serie desde `CAL` hasta `RST` y adjunta:
- Versión firmware (ver `CHANGELOG.md`)
- Valores pMin/pMax/mMin/mMax capturados
- Qué comando `DIR` usaste
- Salida completa de `TUNE` si se ejecutó

---

## 9. Seguridad

⚠️ **ANTES DE CALIBRAR:**
- Asegurar área libre alrededor del actuador/motor
- Tener acceso rápido a botón de parada de emergencia / desconexión 24V
- El sistema tiene Safe State automático (corta PWM + EN=LOW) ante:
  - Overcurrent (sensor A2)
  - Stall (corriente > 950 ADC)
  - Pérdida de señal pedal/feedback

---

**Versión documento:** v2.0.21+ (2026-07-16)
**Firmware compatible:** ACEL_HYUN_V3 v2.0.21+