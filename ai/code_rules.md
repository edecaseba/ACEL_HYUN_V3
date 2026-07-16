# Code Rules — MISRA · ISO · Memoria · Safe State
> Aplican segun el target activo en `ai/hardware_target.json`.
> Cada target define sus reglas especificas en `targets.[active].code_rules`.

---

## MISRA C:2012 / C++:2008 (ambos targets)

- `static_cast` explícito — prohibido casts implícitos de estrechamiento
- Llaves `{}` obligatorias en toda estructura de control (`if`, `for`, `while`, `do-while`)
- Un solo `return` por función (o salidas tempranas limpias al inicio, sin returns dispersos)
- Prohibido: código inalcanzable, variables sin inicializar, macros con efectos secundarios
- Prohibido: `goto` (excepto salida unica de error), `setjmp`/`longjmp`
- Prohibido: `reinterpret_cast`, punteros a funciones no tipados (`void(*)()`)
- Tipos fijos `<cstdint>`: `uint8_t`, `int16_t`, `uint32_t` — nunca `int`, `char` sin typedef
- `constexpr` para constantes en compile-time. `const` para valores inmutables en runtime
- `volatile` obligatorio en toda variable compartida con ISR o acceso a registro HW

---

## Política de Memoria (segun target)

### Arduino Nano (8-bit, 2KB SRAM)
- **Zero Dynamic RAM Post-Init**: prohibido en runtime: `String`, `new`/`delete`, `malloc`/`free`, `realloc`, `calloc`, STL containers (`std::vector`, `std::map`, etc.)
- Solo almacenamiento estático (`static`, variables globales controladas) y pila con tamaño fijo verificado en compile-time
- Pool de memoria estática si se necesita buffer variable: tamaño fijo, preasignado en init, sin alloc/full runtime

### ESP32-S3 (512KB SRAM + 8MB PSRAM)
- **Zero Dynamic RAM Post-Init**: heap (`malloc`, `heap_caps_malloc`) permitido SOLO durante init, antes de `vTaskStartScheduler()`
- Post-init: solo almacenamiento estático, pila de tareas con tamaño fijo, colas/eventos/semáforos creados con API estática (`xQueueCreateStatic`, `xTaskCreateStatic`)
- Prohibido `malloc`/`realloc`/`calloc` despues de que el scheduler este corriendo
- **PSRAM Octal 8MB**: acceso con alineacion 4/8/16 bytes segun tipo de dato. `heap_caps_malloc(MALLOC_CAP_SPIRAM)` solo en init. No usar PSRAM para variables de tiempo real (latencia mayor que SRAM interna)
- **Prevención de fragmentación**: pool de memoria fijo con bloques de tamaño fijo asignados en init. Sin alloc/free en runtime

---

## Safe State (seguridad funcional ISO 13849-1)

### En cualquier fallo crítico
- Todas las salidas de potencia → 0 lógico / alta impedancia inmediatamente
- Flag de error interno (no borrable sin reset intencional)
- Notificar al operador (led, display, UART segun target)

### Fallos que disparan Safe State
| Condición | Acción | Target |
|-----------|--------|--------|
| Pérdida de señal sensor >50ms | EN LOW, motor detenido | Ambos |
| Sobrecorriente (IS_SENSE > umbral) | EN LOW, flag error | Ambos |
| Watchdog expirado | Safe State + reboot con registro | ESP32-S3 |
| Antes de OTA | Safe State obligatorio + esperar confirmacion | ESP32-S3 |
| Fallo de OTA | Mantener Safe State + rollback a particion anterior | ESP32-S3 |

### Secuencia de Boot segura
1. Configurar todos los pines de potencia como GPIOs de salida y ponerlos en LOW (0 lógico)
2. Configurar pines de entrada con pull-up/pull-down segun estado seguro
3. Inicializar periféricos (ADC, PWM, UART, I2C)
4. Verificar alimentación y sensores
5. Solo entonces habilitar potencia (EN → HIGH si todo OK)

---

## ISO 7637-2 (Transitorios automotrices 24V)

### Pulsos a mitigar
| Pulso | Forma | Protección HW | Protección FW |
|-------|-------|---------------|---------------|
| 1: Supresión | -75V, 2ms | TVS SMCJ28CA + C1 470µF | Debounce en ADC |
| 2a: Alternador | +37V, 50µs | TVS + C2 100nF | Filtro EMA |
| 3a/3b: Conmutación | ±75V, 0.1µs | TVS + ferrite beads | Filtro pasa-bajos RC |
| 4: Cranking | <5V, 10ms | C3 100µF | Brownout detection |
| 5: Load dump | +87V, 400ms | TVS SMCJ28CA (clamp @ 45.4V) | Safe State si Vcc < 4.5V |

### Filtrado digital obligatorio
- EMA (Exponential Moving Average) o media móvil en toda lectura de ADC
- Frecuencia de corte: ~1.5kHz (RC = 1kΩ + 100nF)
- Debounce de 10 ciclos en detección de flancos (perdida de señal, sobrecorriente)

---

## ISO 13766 (EMI maquinaria pesada)

- PWM a **20kHz** — mínimo ruido audible, mínima disipación en BTS7960
- Snubber RC (10R 2W + 1nF/1kV) en cada salida de motor a GND
- Ferrite beads axiales THT (600Ω@100MHz, 3A) en serie con cada salida de motor
- Common mode choke (100µH, 2A) en entrada 24V
- Capacitor X7R 1µF entre lineas de 24V justo en el conector de entrada
- Trazas PWM a >5mm de trazas analogicas. Si es inevitable, pista GND entre ellas
- **ESP32-S3**: usar LEDC driver con slew rate control para reducir EMI en bordes PWM

---

## IBT-2 BTS7960 (ambos targets)

### Header 2x4 (2.54mm)
```
Pin 1: RPWM  ──── PWM avance (D10 / GPIO5)
Pin 2: LPWM  ──── PWM retroceso (D9 / GPIO6)
Pin 3: R_EN  ──── Enable forward (D8 / GPIO4)
Pin 4: L_EN  ──── Enable reverse (D8 / GPIO4)
Pin 5: R_IS  ──── Current sense forward → R1(1k) → A2/GPIO3
Pin 6: L_IS  ──── (no usado, dejar NC)
Pin 7: VCC   ──── +5V desde XL4005
Pin 8: GND   ──── GND_PWR
```

### Configuración crítica
- PWM: 20kHz, duty 0-100%, Active High
- **Dead-time: 150ms forzado por firmware** entre cambios de dirección (RPWM→LPWM o LPWM→RPWM). No confiar solo en dead-time interno del BTS7960
- IS_SENSE: 23µA/A typ. R1=1k 1% → 23mV/A. Rango ADC 0-5V. Umbral stall configurable

### Borneras de potencia (en el modulo IBT-2)
- B+ / B- : entrada 24V (desde XL4005 o directo de bateria con proteccion)
- M+ / M- : salida al motor DC

---

## PSRAM Octal (solo ESP32-S3)

### Reglas de layout PCB
- Rutas de datos DQ balanceadas (±10% de longitud)
- Evitar paralelismo con señales de alta frecuencia (>10mm de recorrido paralelo)
- Mantener longitudes iguales dentro del grupo DQ
- Plano GND sólido y sin interrupciones debajo del area PSRAM

### Reglas de firmware
- Alineación: 4 bytes para `uint8_t`/`int8_t`, 8 bytes para `uint16_t`/`int16_t`, 16 bytes para `uint32_t`/`int32_t` y floats
- `heap_caps_malloc(MALLOC_CAP_SPIRAM)` solo en init, antes del scheduler
- No almacenar variables de tiempo real en PSRAM (latencia mayor que SRAM interna)
- Preferir SRAM interna para: buffers de ADC, colas de RTOS, stacks de tareas, variables del PID

---

## Checklist pre-entrega (todo target)

- [ ] **MISRA**: código revisado contra las reglas (static_cast, llaves, tipos fijos, volatile)
- [ ] **Memoria**: sin alloc en runtime. Solo static. Verificado por inspeccion de codigo
- [ ] **Safe State**: todas las salidas van a 0 en cualquier fallo. Boot seguro verificado
- [ ] **Dead-time**: 150ms entre cambios de direccion. Verificado en simulacion
- [ ] **Filtros**: EMA implementado en ADC. Frecuencia de corte ~1.5kHz
- [ ] **Watchdog**: configurado (WDT_2S en Nano, task_wdt en ESP32-S3)
- [ ] **OTA** (solo ESP32-S3): Safe State antes de OTA. Rollback configurado
- [ ] **PSRAM** (solo ESP32-S3): alineacion correcta. Solo alloc en init
- [ ] **Tests**: Unity obligatorios. Cobertura de casos: pedal reposo, perdida señal, stall, dead-time
