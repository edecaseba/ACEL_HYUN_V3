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
├── ai/
│   ├── hardware_target.json  # Config parametrizable del MCU
│   ├── arquitectura_codigo.md
│   ├── hardware_ibt2.md
│   ├── normativas_seguridad.md
│   ├── optimizacion_main_v1.md
│   ├── persona.md
│   └── pipeline.sh
├── .opencode/agents/         # Agentes OpenCode (orquestación IA)
├── opencode.json             # Configuración del equipo de agentes
├── AGENTS.md                 # Reglas del equipo de firmware
├── CHANGELOG.md              # Historial de versiones
└── platformio.ini            # Build configuration
```

## Seguridad Funcional

- ✅ **ISO 13849**: Safe State ante fallos críticos (pérdida de sensor, timeout, sobrecorriente)
- ✅ **ISO 7637-2**: Filtrado digital (EMA) para transitorios automotrices de 24 Vcc
- ✅ **ISO 13766**: PWM a 20 kHz minimizando EMI, cambios de dirección con rampa y dead-time
- ✅ **Boot Security**: Pines de potencia configurados antes de activar enable
- ✅ **Zero Dynamic RAM**: Sin fragmentación de memoria en runtime

## Equipo de Agentes (Orquestación IA)

El proyecto utiliza 7 agentes de IA intercambiables mediante **Pool Universal de Fallback**:

| Agente | Modelo | Función |
|--------|--------|---------|
| @orchestrator | Gemini 3.1 Pro | Orquestación + autorización |
| @planner | North Mini Code Free | Planificación técnica |
| @coder | DeepSeek V4 Flash Free | Programación C/C++ |
| @reviewer | Mimo V2.5 Free | Revisión (veto absoluto) |
| @tester | North Mini Code Free | Tests y procedimientos |
| @documenter | Mimo V2.5 Free | Documentación |
| @explore | Gemini 3.5 Flash | Exploración de código |

Si un agente se queda sin tokens, otro agente con modelo diferente toma su lugar automáticamente.

## Requisitos de Desarrollo

- **IDE**: VS Code + PlatformIO
- **Toolchain**: AVR GCC (PlatformIO)
- **Lenguaje**: C++17 / C++20
- **Tests**: Unity Test Framework + ArduinoFake

## Regla de Oro

> **Si todo está implementado, probado, revisado y documentado:**
> 1. Hacer commit con todos los cambios a `main`
> 2. Pushear al repositorio remoto en Gitea
> 3. Si el repositorio no existe en Gitea, crearlo automáticamente antes del push
> 4. Verificar que el remoto esté configurado correctamente
> 5. No pushear código con tests fallando, revisión rechazada, o documentación incompleta
>
> *Esta regla aplica al finalizar cada sesión de desarrollo exitosa.*

## Licencia

Uso interno — maquinaria industrial pesada.
