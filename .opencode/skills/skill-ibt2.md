---
name: skill-ibt2
description: Especificaciones del modulo IBT-2 BTS7960. Pinout, dead-time, PWM, IS_SENSE, snubber, esquematico de conexion. Cargar solo cuando se trabaje con la etapa de potencia.
---

# IBT-2 Module (BTS7960) Reference

## Pinout header 2x4 (2.54mm)
```
 1 RPWM  ──── D10 (PWM avance, Active High)
 2 LPWM  ──── D9  (PWM retroceso, Active High)
 3 R_EN  ──── D8  (Enable forward, Active High)
 4 L_EN  ──── D8  (Enable reverse, Active High)
 5 R_IS  ──── A2 via R1(1k); C5(100nF)→GND
 6 L_IS  ──── (no usado)
 7 VCC   ──── +5V (desde XL4005)
 8 GND   ──── GND_PWR
```

## Borneras potencia
- B+ / B- : alimentación 24V (desde fuente)
- M+ / M- : motor acelerador

## Configuración crítica
- **PWM frequency:** 20kHz (máx 25kHz según datasheet)
- **Dead-time:** 150ms forzado por firmware entre cambio de dirección (no confiar solo en dead-time interno del BTS7960)
- **IS_SENSE:** corriente ≈ 23µA/A de carga (R1=1k da 23mV/A). Rango ADC: 0-5V.
- **Snubber RC:** 10R 2W + 1nF/1kV en paralelo con cada salida a GND
- **Ferrite beads:** axial THT 600Ω@100MHz 3A en serie con cada salida

## Alimentación
- VCC: +5V desde XL4005. NO alimentar desde Arduino 5V pin (corriente insuficiente)
- GND: GND_PWR (NO GND_ANA)

## Protecciones integradas (BTS7960)
- Overtemperature shutdown (latch)
- Overcurrent limit (43A typ, switched mode)
- Undervoltage shutdown
- Short circuit protection
