# ACEL_HYUN_V3 — Controlador de Acelerador para Excavadora Hyundai R250 LC7

Controlador electrónico de acelerador para excavadora Hyundai R250 LC7 con puente H IBT-2 (BTS7960), control PID en lazo cerrado y cumplimiento de normativas de seguridad industrial ISO 13849, ISO 7637-2 e ISO 13766.

---

## Especificaciones de Hardware

| Parámetro | Valor |
|-----------|-------|
| MCU | Arduino Nano (ATmega328P) @ 16 MHz |
| Framework | Arduino 2.3.0 |
| Alimentación | 24 Vcc |
| Flash | 32 KB |
| SRAM | 2 KB |
| Política de memoria | Zero Dynamic RAM (solo estática post-init) |
| Driver de potencia | IBT-2 BTS7960 (Puente H) |
| PWM | 20 kHz con dead-time configurable |
| Estándar de código | MISRA C:2012 / MISRA C++:2008 |

## Pinout

| Pin | Función | Descripción |
|-----|---------|-------------|
| A0 | PotOp | Potenciómetro de operación (pedal) |
| A1 | PotFeed | Potenciómetro de retroalimentación (acelerador) |
| A2 | IS_SENSE | Sensor de corriente (detección de stall) |
| D8 | EN | Enable del BTS7960 |
| D9 | L_PWM | PWM de retroceso |
| D10 | R_PWM | PWM de avance |

## Arquitectura del Sistema

```
┌──────────┐    ┌──────────────┐    ┌───────────┐    ┌──────────┐
│ Pedal    │───►│ Filtro EMA   │───►│ PID      │───►│ BTS7960  │──► Motor
│ (ADC)    │    │ Anti-aliasing│    │ Controller│    │ (PWM)    │
└──────────┘    └──────────────┘    └───────────┘    └──────────┘
                      ▲                                  │
                      │                                  ▼
                      │                           ┌──────────┐
                      └───────────────────────────┤ Feedback │
                                                  │ (ADC)    │
                                                  └──────────┘
```

- **Filtro EMA**: Suavizado de lecturas analógicas para cumplir ISO 7637-2 (transitorios automotrices)
- **PID**: Control discreto con anti-windup, límite de slew rate y reset por fricción
- **BTS7960**: Dead-time obligatorio de 150 ms entre cambios de dirección (anti shoot-through)
- **Safe State**: Ante pérdida de señal o sobrecorriente, EN → LOW, motor detenido

## Estructura del Proyecto

```
├── src/
│   ├── main.cpp              # Lógica principal con PID integral
│   └── pid_controller.h      # Controlador PID discreto
├── test/
│   ├── test_pid.cpp          # Tests unitarios PID (Unity)
│   └── TEST_PROCEDURE.md     # Procedimiento de prueba en hardware
├── include/                  # Librerías adicionales
├── lib/                      # Dependencias
└── platformio.ini            # Build configuration
```

## Seguridad Funcional

- ✅ **ISO 13849**: Safe State ante fallos críticos (pérdida de sensor, timeout, sobrecorriente)
- ✅ **ISO 7637-2**: Filtrado digital (EMA) para transitorios automotrices de 24 Vcc
- ✅ **ISO 13766**: PWM a 20 kHz minimizando EMI, cambios de dirección con rampa y dead-time
- ✅ **Boot Security**: Pines de potencia configurados antes de activar enable
- ✅ **Zero Dynamic RAM**: Sin fragmentación de memoria en runtime

## Requisitos de Desarrollo

- **IDE**: VS Code + PlatformIO
- **Toolchain**: AVR GCC (PlatformIO)
- **Lenguaje**: C++17 / C++20
- **Tests**: Unity Test Framework + ArduinoFake

## Regla de Oro

> **Electronic Engineering Partner — Especializado en Maquinaria Pesada.**
>
> Este repositorio no solo contiene firmware: también incluye especificaciones
> de hardware, esquemáticos de filtrado, layout de PCB y estrategias EMC/EMI
> para garantizar funcionamiento en entornos industriales de alta interferencia
> electromagnética (24Vcc, hidráulica, motores de alto torque, transitorios
> automotrices ISO 7637-2). Todo cambio de hardware debe documentarse en los
> archivos de la carpeta `hardware/` y validarse contra las normativas
> ISO 13849, ISO 7637-2 e ISO 13766.

## Licencia

Uso interno — maquinaria industrial pesada.
