---
name: motor-startup-diagnostics
description: Historial de causas raiz encontradas y corregidas para fallas al arrancar el motor del acelerador (reset del micro, autoridad de PWM limitada, corrupcion de EEPROM). Cargar antes de investigar cualquier reporte de "el motor no arranca", "el micro se reinicia", comportamiento erratico al primer movimiento, o EEPROM/calibracion que se pierde sola.
---

# Diagnostico: fallas de arranque del motor — historial de causa raiz

Este documento existe para no re-derivar desde cero un diagnostico ya hecho. Antes de investigar un reporte nuevo de arranque del motor, revisar si encaja con alguno de estos patrones ya confirmados.

## Caso: "el micro se reinicia al arrancar el motor" (v2.0.23 → fix en v2.0.24)

Sintoma reportado: el ATmega328P se resetea especificamente cuando el motor arranca a moverse (no en reposo).

Causas raiz encontradas, en orden de probabilidad segun evidencia:

1. **PWM del puente H a 62.5kHz** (`initMotorHardware()`, Timer1 modo 8-bit TOP=0xFF sin prescaler). El IBT-2/BTS7960 soporta maximo 25kHz (ver skill `ibt2-bts7960`). Correr por encima del limite del driver genera transitorios/EMI capaces de resetear el MCU justo cuando el PWM empieza a conmutar — coincide con el sintoma exacto (reset solo al arrancar, no en reposo).
2. **Colision de direcciones EEPROM**: `EE_NOMINAL_ADDR`/`EE_SIGMA_ADDR` (calibracion de sobrecorriente, `overcurrent.h`) arrancaban en addr `0x00`, la misma direccion donde `eepromGetSafe/PutSafe(0, cfg)` persiste la config general (`main.cpp`). Cada guardado de una pisaba bytes de la otra. Un `cfg.pMin` corrupto produce un error de setpoint enorme al bootear → el PID manda PWM al maximo desde el primer ciclo → pico de corriente → puede disparar el mismo reset.
3. **Falta de soft-start**: sin rampa, el PID salta de 0 a `PWM_MIN_OPERACION=90` (35% duty) de forma instantanea apenas el error sale de la zona muerta. Un soft-start anterior se habia sacado en v2.0.23 por usar `delay()` bloqueante (rompia el watchdog) — el problema era la implementacion, no la idea.

Fix aplicado (v2.0.24):
- PWM revertido a 20kHz, 10-bit (`ICR1=799`, modo 14 real — ver detalle de bug de modo abajo)
- `pwmWriteMotor()` reemplaza `analogWrite()`, escala 0-255→0-799 escribiendo `OCR1A`/`OCR1B` directo (analogWrite asumia TOP=255 y limitaba la autoridad real a ~32%)
- `EE_NOMINAL_ADDR` movido a partir de addr 64, con `static_assert(sizeof(Config) <= EE_NOMINAL_ADDR)` en `main.cpp` para que no vuelva a colisionar si `Config` crece
- `aplicarRampa()`: soft-start no bloqueante (sin `delay()`), 25 unidades de PWM por ciclo de `loop()`, se omite en `tuningLimitCycle`
- `MCUSR` se lee y reporta por Serial (`BROWNOUT`/`WATCHDOG`/`EXTERNAL`/`POWER-ON`) antes de limpiarse en `setup()`, para confirmar en banco la causa real si el sintoma reaparece

## Bug independiente encontrado: modo Timer1 incorrecto (preexistia, incluso en la version "20kHz buena")

`TCCR1A`/`TCCR1B` armaban `WGM13:10 = 1111` (modo 15, Fast PWM, **TOP=OCR1A**) en vez de `1110` (modo 14, **TOP=ICR1**). En modo 15, `OCR1A` es simultaneamente el TOP del periodo del timer Y el duty del canal L_PWM (pin 9) — cualquier cambio de duty en L_PWM cambia el periodo de todo el timer, afectando tambien a R_PWM. Corregido junto con el fix de frecuencia: `TCCR1A` solo debe setear `WGM11` (no `WGM10`).

**Regla para el futuro:** si se vuelve a tocar `initMotorHardware()`, verificar contra la tabla de modos Waveform Generation del datasheet ATmega328P (seccion Timer1) que `WGM13:10` sea exactamente `1110`, no asumir por el nombre del modo.

## Como verificar en banco si el sintoma reaparece
1. Leer el mensaje `Reset cause (MCUSR)` que imprime `setup()` por Serial apenas bootea — indica si fue BROWNOUT, WATCHDOG, EXTERNAL o POWER-ON.
2. Si es BROWNOUT: sospechar inrush de corriente (revisar snubber/ferrites en la placa real, no solo firmware) o alimentacion 5V insuficiente durante el pico de arranque del motor.
3. Si es WATCHDOG: buscar una ruta con `delay()` o un bucle largo sin `wdt_reset()` (ej. una calibracion nueva mal escrita).
4. Confirmar con osciloscopio/multimetro que la frecuencia real del PWM en D9/D10 es ~20kHz, no un multiplo raro — un futuro cambio a `initMotorHardware()` podria reintroducir el bug de modo 15.
