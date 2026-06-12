# ACEL_HYUN_V3 — Controlador de Acelerador para Excavadora Hyundai R250 LC7

Controlador electrónico de acelerador para excavadora Hyundai R250 LC7 con puente H IBT-2 (BTS7960), control PID en lazo cerrado y cumplimiento de normativas de seguridad industrial ISO 13849, ISO 7637-2 e ISO 13766.

---

## Especificaciones de Hardware

| Parámetro | Valor |
|-----------|-------|
| MCU | Arduino Nano (ATmega328P) @ 16 MHz |
| Framework | Arduino 2.3.0 |
| Alimentación | 24 Vcc |
| Fuente | Módulo Step-Down XL4005 (24V → 5V) |
| Flash | 32 KB |
| SRAM | 2 KB |
| Política de memoria | Zero Dynamic RAM (solo estática post-init) |
| Driver de potencia | Módulo IBT-2 BTS7960 (Puente H, 43A) |
| PWM | 20 kHz con dead-time configurable |
| Estándar de código | MISRA C:2012 / MISRA C++:2008 |

## Pinout

| Pin | Función | Descripción |
|-----|---------|-------------|
| A0 | PotOp | Potenciómetro de operación (pedal) |
| A1 | PotFeed | Potenciómetro de retroalimentación (acelerador) |
| A2 | IS_SENSE | Sensor de corriente (detección de stall) |
| D8 | EN | Enable del IBT-2 (R_EN + L_EN) |
| D9 | L_PWM | PWM de retroceso |
| D10 | R_PWM | PWM de avance |

## Arquitectura del Sistema

```
┌──────────┐    ┌──────────────┐    ┌───────────┐    ┌──────────┐
│ Pedal    │───►│ Filtro EMA   │───►│ PID      │───►│ IBT-2   │──► Motor
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
- **IBT-2 (BTS7960)**: Dead-time obligatorio de 150 ms entre cambios de dirección (anti shoot-through)
- **Safe State**: Ante pérdida de señal o sobrecorriente, EN → LOW, motor detenido

## Estructura del Proyecto

```
ACEL_HYUN_V3/
├── src/
│   ├── main.cpp              # Lógica principal con PID integral
│   └── pid_controller.h      # Controlador PID discreto
├── test/
│   ├── test_pid.cpp          # Tests unitarios PID (Unity)
│   └── TEST_PROCEDURE.md     # Procedimiento de prueba en hardware
├── include/                  # Librerías adicionales
├── lib/                      # Dependencias
├── hardware/                 # <privado> Documentación + diseño PCB
│   ├── BOM.csv               # Lista de materiales THT
│   ├── PCB_DESIGN_GUIDE.md   # Guía de diseño + esquemático + layout
│   └── KiCAD_SETUP.md        # Configuración KiCad / footprints / netlist
├── platformio.ini            # Build configuration
├── gitea-init.sh             # Script de inicialización Gitea
└── README.md                 # Este archivo
```

> 🔒 **hardware/** es privado y está excluido del repositorio público vía `.gitignore`.
> Solo existe localmente en la notebook de desarrollo. No se sube a Gitea.

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

## Licencia

Uso interno — maquinaria industrial pesada.
