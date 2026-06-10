# Change Log — Optimización ACEL_HYUN_V3

## Fecha
2026-06-10

## Archivo modificado
- `src/main.cpp` — Reestructuración completa.

---

## 1. Filtrado Digital EMA (Exponential Moving Average)

Se implementó `EMAFilter` como struct con métodos `init()` y `update()` para suprimir ruido automotriz de 24Vcc (ISO 7637-2) en las tres señales analógicas críticas:

| Señal | Alpha | Constante de tiempo efectiva |
|-------|-------|------------------------------|
| Pedal operador | 0.15 | ~6.7 muestras |
| Feedback actuador | 0.20 | ~5.0 muestras |
| Corriente (IS sense) | 0.30 | ~3.3 muestras |

Los filtros se actualizan en cada ciclo de `loop()`, y toda lógica de control consume el valor filtrado (`filteredValue`) en lugar del raw ADC.

## 2. Arquitectura 100% Asíncrona (Zero delay())

Eliminados todos los `delay()` bloqueantes. Reemplazados por máquina de estados con marcas de tiempo (`millis()`):

| Estado anterior (delay) | Reemplazo |
|--------------------------|-----------|
| `delay(4000)` en calibración de pedal | Estados `WAIT_PEDAL_MIN`/`READ_PEDAL_MIN`/`WAIT_PEDAL_MAX`/`READ_PEDAL_MAX` con marcas de tiempo |
| `while` + `delay(20)` en `buscarTopeMotor()` | Estados `RUN_MOTOR_MIN`/`RUN_MOTOR_MAX` con `tEstable` + timeout de 10s |
| `delay(1500)` entre calibraciones de motor | Transición directa entre estados |

El loop principal nunca se bloquea, garantizando latencia < 2ms.

## 3. Dead-Time No Bloqueante (BTS7960)

La inyección de dead-time en `mover()` ahora usa el flag `deadTimeActive` + `deadTimeUntil`:

- Ante un cambio de dirección, se llama a `detener()` y se programa la espera de 150ms.
- En cada ciclo de `loop()` se verifica si el dead-time expiró antes de permitir el nuevo movimiento.
- La CPU nunca se bloquea durante la espera.

## 4. Máquina de Estados de Calibración

Se implementó `CalState` con 10 estados (`IDLE` → `WAIT_PEDAL_MIN` → `READ_PEDAL_MIN` → `WAIT_PEDAL_MAX` → `READ_PEDAL_MAX` → `CAL_MOTOR_MIN` → `RUN_MOTOR_MIN` → `CAL_MOTOR_MAX` → `RUN_MOTOR_MAX` → `SAVE`) que reemplaza la función bloqueante `ejecutarCalibracion()`.

## 5. Cumplimiento MISRA y Zero Dynamic RAM

- `static_cast` explícito en todas las conversiones entre tipos.
- Tipos de ancho fijo (`<stdint.h>`): `uint8_t`, `int16_t`, `uint16_t`, `uint32_t`.
- Llaves obligatorias en todas las estructuras de control.
- Uso de `nullptr` en lugar de `NULL`.
- `constexpr` para todas las constantes de configuración.
- Sin `String`, `new`, `delete`, `malloc`, `free` — Zero Dynamic RAM post-init.

## Métricas de compilación

| Recurso | Uso | Capacidad |
|---------|-----|-----------|
| RAM     | 275B (13.4%) | 2048B |
| Flash   | 6786B (22.1%) | 30720B |
