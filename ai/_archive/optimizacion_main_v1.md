# Plan de Optimización y Cumplimiento - ACEL_HYUN_V3 (Final)

Este plan detalla la reestructuración completa de `src/main.cpp` para cumplir con las directivas de seguridad funcional, arquitectura de memoria (Zero Dynamic RAM) y estándares MISRA.

## Objetivos Finales
1.  **Filtrado Digital EMA**: Implementación de filtros de media móvil exponencial para suprimir ruido de 24V en pedal, feedback y sensor de corriente.
2.  **Arquitectura 100% Asíncrona**: Eliminación total de `delay()` mediante una máquina de estados centralizada.
3.  **Cumplimiento MISRA**: Uso de `static_cast`, tipos de ancho fijo (`<cstdint>`) y lógica determinista.
4.  **Hardware Safety (BTS7960)**: Gestión de dead-time no bloqueante para protección del puente H.

## Estructura de Datos y Tipos
```cpp
struct EMAFilter {
    float alpha;
    float filteredValue;
    void update(float newValue) {
        filteredValue = (alpha * newValue) + (1.0f - alpha) * filteredValue;
    }
};

enum class SystemState : uint8_t {
    INIT,
    CALIBRATING_PEDAL_MIN,
    CALIBRATING_PEDAL_MAX,
    CALIBRATING_MOTOR_MIN,
    CALIBRATING_MOTOR_MAX,
    RUNNING,
    FAULT_STALL,
    SAFE_STATE
};
```

## Cambios en la Lógica de Control
- **`loop()`**: Actuará como el despachador de la máquina de estados.
- **`monitorStallCurrent()`**: Integrado en el ciclo de control con filtrado EMA para evitar falsos positivos por picos transitorios.
- **`mover()`**: Implementará una guarda de tiempo para el cambio de dirección sin bloquear la CPU.

## Pasos de Implementación
1.  **Definición de Constantes y Filtros**: Configurar alfas de filtrado según la dinámica de cada señal.
2.  **Refactorización de Capa de Potencia**: Reescribir `mover()` para usar marcas de tiempo.
3.  **Implementación de Máquina de Estados**: Migrar la lógica de calibración a los estados correspondientes.
4.  **Limpieza MISRA**: Revisar cada expresión para asegurar casts explícitos y llaves obligatorias.

## Verificación
- Monitoreo por Serial de la latencia del loop (objetivo < 2ms).
- Validación de la transición a `SAFE_STATE` ante desconexión de sensores o sobrecorriente.
