---
name: embedded-safety-rules
description: Reglas MISRA C++:2008, ISO 13849-1/7637-2/13766, politica de memoria y Safe State para firmware embebido de ACEL_HYUN_V3 (Arduino Nano ATmega328P, unico target). Cargar antes de escribir, revisar o auditar cualquier codigo en src/.
---

# Reglas de firmware — ACEL_HYUN_V3

Target unico y definitivo: Arduino Nano ATmega328P (ver `ai/hardware_target.json`). No hay ni habra migracion a otro MCU/lenguaje — decision explicita del usuario, no es seguro en esta clase de microcontroladores para firmware de seguridad funcional. No proponer targets alternativos.

## MISRA C:2012 / C++:2008
- `static_cast` explicito — prohibido casts implicitos de estrechamiento
- Llaves `{}` obligatorias en toda estructura de control (`if`, `for`, `while`, `do-while`)
- Prohibido: codigo inalcanzable, variables sin inicializar, macros con efectos secundarios
- Prohibido: `goto` (salvo salida unica de error), `setjmp`/`longjmp`, `reinterpret_cast`, punteros a funcion no tipados
- Tipos fijos `<cstdint>`: `uint8_t`, `int16_t`, `uint32_t` — nunca `int`/`char` sin typedef
- `constexpr` para constantes compile-time, `const` para inmutables runtime
- `volatile` obligatorio en toda variable compartida con ISR o registro HW

## Memoria (8-bit, 2KB SRAM)
- **Zero Dynamic RAM post-init**: prohibido en runtime `String`, `new`/`delete`, `malloc`/`free`/`realloc`/`calloc`, contenedores STL
- Solo almacenamiento estatico y pila de tamano fijo verificado en compile-time
- Buffers variables: pool estatico preasignado en init, sin alloc en runtime

## Safe State (ISO 13849-1)
Ante cualquier fallo critico: salidas de potencia → 0 logico/alta impedancia inmediato, flag de error no auto-borrable, notificar al operador.

Disparan Safe State: perdida de señal sensor >50ms (`checkSignalLoss()` en `main.cpp`, logica pura en `signal_loss.h` — necesario porque el actuador NO tiene finales de carrera fisicos, ver `motor-startup-diagnostics`), sobrecorriente (IS_SENSE > umbral), watchdog expirado, stall mecanico detectado.

Secuencia de boot segura: 1) pines de potencia como salida en LOW, 2) pines de entrada con pull-up/pull-down en estado seguro, 3) inicializar perifericos, 4) verificar alimentacion/sensores, 5) recien ahi habilitar potencia (EN → HIGH).

## ISO 7637-2 (transitorios automotrices 24V)
| Pulso | Forma | Mitigacion FW |
|-------|-------|----------------|
| 1 Supresion | -75V, 2ms | Debounce en ADC |
| 2a Alternador | +37V, 50µs | Filtro EMA |
| 3a/3b Conmutacion | ±75V, 0.1µs | Filtro pasa-bajos RC (HW) |
| 4 Cranking | <5V, 10ms | Brownout detection |
| 5 Load dump | +87V, 400ms | Safe State si Vcc < 4.5V |

Filtrado digital obligatorio: EMA en toda lectura ADC, fc ≈ 1.5kHz (alphas actuales: pedal 0.75, feedback 0.80, corriente 0.85).

## ISO 13766 (EMI maquinaria pesada)
- PWM del puente H a **20kHz** (max 25kHz por datasheet IBT-2/BTS7960) — ver skill `ibt2-bts7960`
- Trazas PWM a >5mm de trazas analogicas; si es inevitable, pista GND entre ellas

## Checklist antes de dar por cerrado un cambio en src/
1. MISRA: static_cast, llaves, tipos fijos, volatile en ISR
2. Memoria: sin alloc en runtime (inspeccion de codigo)
3. Safe State: toda salida va a 0 en cualquier fallo; boot seguro
4. Dead-time: 150ms entre cambios de direccion del puente H
5. Watchdog: `wdt_enable(WDTO_2S)` en `setup()`, `wdt_reset()` en cada `loop()` y en esperas largas (calibracion)
6. Tests: `pio test -e native` — todo verde antes de dar el cambio por bueno
7. Build real: `pio run -e nanoatmega328` limpio con `-Werror`, revisar RAM/Flash reportado
