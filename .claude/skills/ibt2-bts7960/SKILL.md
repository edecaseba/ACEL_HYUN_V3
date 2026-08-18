---
name: ibt2-bts7960
description: Especificaciones de hardware del modulo IBT-2 (BTS7960) que maneja el motor DC del acelerador. Pinout, limite real de frecuencia PWM, dead-time, sensado de corriente, protecciones internas. Cargar solo al tocar la etapa de potencia (initMotorHardware, pwmWriteMotor, mover, overcurrent).
---

# IBT-2 (BTS7960) — referencia de hardware

## Pinout header 2x4 (2.54mm)
```
1 RPWM ── D10 (PIN_R_PWM, Timer1/OC1B, Active High)
2 LPWM ── D9  (PIN_L_PWM, Timer1/OC1A, Active High)
3 R_EN ── D8  (PIN_EN, Enable forward)
4 L_EN ── D8  (PIN_EN, Enable reverse — unido a R_EN)
5 R_IS ── A2 via R1(1k); C5(100nF)→GND  (PIN_IS_SENSE)
6 L_IS ── no usado
7 VCC  ── +5V desde XL4005 (NO desde el pin 5V del Arduino: corriente insuficiente)
8 GND  ── GND_PWR (no GND_ANA)
```
Borneras de potencia: B+/B- = 24V, M+/M- = motor del acelerador.

## Limite de frecuencia PWM — el mas importante
**Maximo 25kHz segun datasheet BTS7960.** El firmware corre a **20kHz** (Timer1 Fast PWM modo 14, TOP=ICR1=799, prescaler 1, `16MHz/800=20kHz`).

Incidente real (v2.0.23 → corregido en v2.0.24): una edicion subio el PWM a 62.5kHz (TOP=0xFF, 8-bit) para que `analogWrite()` tuviera rango 0-255 completo. A esa frecuencia el driver BTS7960 malfunciona y genera transitorios que **resetean el ATmega328P** al arrancar el PWM. Si alguna vez se vuelve a tocar `initMotorHardware()`: mantener `ICR1=799`, prescaler 1, modo 14. No usar modo 15 (WGM10=1): en ese modo TOP=OCR1A, que es tambien el duty del canal L_PWM — se rompen ambos al mismo tiempo.

Para escribir duty sin perder autoridad (analogWrite() asume TOP=255 y con TOP=799 tope la potencia real en ~32%): usar `pwmWriteMotor()` en `main.cpp`, que escala 0-255 → 0-799 y escribe `OCR1A`/`OCR1B` directo.

## Dead-time
**150ms forzado por firmware** (`DEAD_TIME_MS`) entre cambios de direccion (RPWM→LPWM o viceversa). No confiar solo en el dead-time interno del BTS7960. Se omite unicamente en `firstMovementAfterStop` (ambos PWM en 0 = seguro) y durante `tuningLimitCycle` (auto-tune bang-bang controlado).

## Sensado de corriente (IS_SENSE)
~23µA/A de carga; R1=1kΩ → 23mV/A. Rango ADC 0-5V. `STALL_CURRENT_ADC` y la calibracion de sobrecorriente (`overcurrent.h/.cpp`) usan este canal. La calibracion se persiste en EEPROM desde `EE_NOMINAL_ADDR` (ver nota de colision de direcciones en `motor-startup-diagnostics`).

## Protecciones integradas en el BTS7960
Overtemperature shutdown (latch), overcurrent limit (~43A typ, switched mode), undervoltage shutdown, short circuit protection. Estas son la ultima linea de defensa — el firmware no debe depender de ellas como mecanismo primario (ver `STALL_CURRENT_ADC` y dead-time).

## Mitigacion EMI (hardware, no firmware — verificar que este presente en la placa real)
Snubber RC 10Ω 2W + 1nF/1kV en paralelo con cada salida a GND. Ferrite beads axiales THT 600Ω@100MHz 3A en serie con cada salida. TVS SMCJ28CA en la entrada de 24V.
