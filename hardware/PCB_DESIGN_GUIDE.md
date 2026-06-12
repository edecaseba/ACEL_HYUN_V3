# Guía de Diseño PCB — ACEL_HYUN_V3
## Controlador de Acelerador — Excavadora Hyundai R250 LC7

> **Electronic Engineering Partner — Especializado en Maquinaria Pesada.**
> Compatibilidad electromagnética según ISO 13766, transitorios ISO 7637-2,
> seguridad funcional ISO 13849.

---

## Índice

1. [Diagrama Esquemático General](#1-diagrama-esquematico-general)
2. [Etapa de Potencia — BTS7960](#2-etapa-de-potencia--bts7960)
3. [Etapa de Alimentación — 24V a 5V/3.3V](#3-etapa-de-alimentacion)
4. [Acondicionamiento de Señales Analógicas](#4-acondicionamiento-de-senales-analogicas)
5. [Filtrado EMI — Entradas y Salidas](#5-filtrado-emi)
6. [Layout de PCB — Reglas de Ruteo](#6-layout-de-pcb)
7. [Lista de Componentes (BOM Sugerida)](#7-lista-de-componentes)
8. [Procedimiento de Validación](#8-procedimiento-de-validacion)

---

## 1. Diagrama Esquemático General

```
                          +-----------------------------------------+
                          |  ETAPA DE ALIMENTACION                   |
  24V_BAT+ ──┬── F1 ── D1 ── C1 ── C2 ── L1 ── C3 ── C4 ── U1 ── +5V
             │    3A    TVS   470µF  100nF   CM  100µF  100nF   7805  │
             │           SMCJ28CA        choke                         │
  24V_BAT- ──┴───────────────────────────────────────────────────── GND
                                                                       │
                                                                      +3.3V
                                                                       │
                                                                     U2(AMS1117)
                                                                       │
                                                                      GND_PWR
                          +-----------------------------------------+
                          |  ETAPA DE POTENCIA (BTS7960)             |
                          |                                          |
  +5V ────────────────────┤ VCC                                      │
                          │                                          │
  PIN_EN (D8) ────────────┤ EN                                  OUT │──── R_PWM_OUT ────┬─── F2 ──── J1_MOTOR_A
  PIN_R_PWM (D10) ────────┤ RPWM                                   │                    │    ferrite
  PIN_L_PWM (D9) ─────────┤ LPWM                              OUT+  │──── L_PWM_OUT ────┬─── F3 ──── J1_MOTOR_B
                          │                                     OUT- │                    │    ferrite
  A2 ─── R1 ─── C5 ──────┤ IS_SENSE                                │                    │
        1k    100nF        │                                          │                    └─── SNUBBER RC
                          │                                          │                         R_snub=10Ω 1W
  GND_PWR ────────────────┤ GND                                      │                         C_snub=1nF 1kV
                          +------------------------------------------+

                          +------------------------------------------+
                          |  ACONDICIONAMIENTO ANALOGICO             |
                          |                                          |
  J2_PEDAL_SIG ──┬─── F4 ──┬─── R2 ── C6 ──┬─── D2 ──┬─── A0 (MCU)
                 │  ferrite │   1kΩ   100nF  │    BAT54  │
                 │  (BLM21) │                │  (clamp)  │
                 │          │                │           │
                 │          ▼                ▼           ▼
                 │         GND_ANA          GND_ANA    GND_ANA
                 │
                 │         ┌─── R3 ── C7 ──┬─── D3 ──┬─── A1 (MCU)
  J2_FEED_SIG ───┴─── F5 ──┤   1kΩ   100nF  │   BAT54  │
                    ferrite │                │  (clamp)  │
                    (BLM21) │                │           │
                            ▼                ▼           ▼
                           GND_ANA          GND_ANA    GND_ANA

                          +------------------------------------------+
                          |  DIVISOR DE POTENCIOMETROS               |
                          |                                          |
  +3.3V ─── C8 ───────┬─── J2_PEDAL_VCC (+3.3V al sensor)           │
         100nF/50V     │                                             │
                       │                                             │
  +3.3V ─── C9 ───────┴─── J2_FEED_VCC (+3.3V al sensor)            │
         100nF/50V                                                    │
                                                                      │
                          +------------------------------------------+
                          |  DECOUPLING MCU (Arduino Nano)           |
                          |                                          |
  +5V ─── R4 ──┬─── C10 ── C11 ── U? (Nano VIN)
         10Ω   100µF   100nF
                │       │
                ▼       ▼
               GND     GND

                          +------------------------------------------+
                          |  SWITCH DE RESET EXTERNO                 |
                          |                                          |
  RESET_N ───────┬─── SW1 ─── GND
                 │     (tactil)
                 │
                 └─── R5 ─── +5V  (pull-up externa 10kΩ)
```

---

## 2. Etapa de Potencia — BTS7960

### 2.1 Protección de Salidas (Motor)

El BTS7960 conmuta a 20 kHz, lo que genera picos de alta frecuencia en los
bornes del motor. Se requieren:

```
                    ┌──────┐
  R_PWM_OUT ───────┤ F2   ├──────┬─────── J1_A (Motor A)
                    │ FB   │      │
                    └──────┘      │
                                  ├─── R_snub ──┐
                                             │
                                  ├─── C_snub ──┘
                                             │
  GND_PWR ────────────────────────────────┴─── GND_PLANE
```

**Componentes:**
- **F2, F3**: Ferrite beads SMD 1206, 600Ω @ 100MHz, 2A (BLM31PG601SN1)
- **R_snub**: 10Ω 1W carbon composition o metal film (para pulsos)
- **C_snub**: 1nF 1kV cerámico NP0/C0G (baja ESR a altas frecuencias)

### 2.2 Sensing de Corriente (IS_SENSE)

El pin IS_SENSE del BTS7960 entrega una corriente proporcional a la carga.
Se convierte a voltaje con una resistencia externa:

```
  BTS7960_IS ──── R1(1kΩ 1%) ────┬──── A2 (MCU)
                                  │
                                  └── C5(100nF X7R) ──── GND_PWR
```

**Recomendación layout:** R1 lo más cerca posible del pin IS_SENSE del
BTS7960. C5 cerca del pin A2 del Nano.

### 2.3 Dead-Time por Hardware (Opcional)

El firmware ya implementa 150ms de dead-time, pero se puede agregar
protección adicional con red RC + Schmitt Trigger en los gate drives
para prevenir shoot-through incluso si el firmware falla.

---

## 3. Etapa de Alimentación

### 3.1 Protección de Entrada 24V

```
  24V_IN ──┬── F1(3A PTC) ── D1(TVS) ── C1 ── C2 ── L1 ── C3 ── C4 ── U1 ── +5V
           │                    │         470µF 100nF      100µF  100nF  7805   │
           │                    │                                               │
           │                    ▼                                               │
  24V_GND ─┴──────────────────── GND_PWR ──────────────────────────────────────┘
```

**D1 — TVS SMCJ28CA:**
- Protección bidireccional contra transitorios ISO 7637-2
- Pulsos de load dump (hasta 87V durante 400ms)
- Clamping @ 45.4V, pico de pulso 300W

**L1 — Common Mode Choke:**
- 100µH, 2A, tipo through-hole o SMD grande (Würth 744823310)
- Suprime ruido de modo común del sistema hidráulico

**C1, C3 — Electrolíticos:**
- C1: 470µF/50V, baja ESR (Panasonic FR o Nichicon PW)
- C3: 100µF/35V, misma serie
- Soportan ripple de la conmutación 20kHz

**C2, C4 — Cerámicos:**
- 100nF/50V X7R, MLCC
- Eliminan ruido de alta frecuencia antes y después del choke

**U1 — Regulador 7805:**
- Versión automotriz (LM2940T-5.0 o similar)
- Dropout bajo (<0.5V)
- Soportar hasta 40V entrada
- Disipador térmico obligatorio (I_motor ~1.5A → P_dis ~ 30W)
- **Alternativa:** convertidor DC-DC (TPS54360) para >90% eficiencia

### 3.2 Regulación a 3.3V (para sensores)

```
  +5V ──── C4A ──── U2(AMS1117-3.3) ──── C5A ──── +3.3V
          10µF                        10µF        │
          tant.                       tant.        │
                                                   └─── C5B ──── GND
                                                        100nF
```

**Aislar plano GND_ANA del GND_PWR** mediante un ferrite bead de baja
frecuencia (BLM21BD102SN1, 1kΩ @ 100MHz) para no acoplar ruido de
potencia a los sensores.

---

## 4. Acondicionamiento de Señales Analógicas

### 4.1 Filtro Pasa-Bajos + Protección

Cada entrada de potenciómetro (pedal y feedback) lleva:

```
  Sensor ──┤ FB ├── R2(1kΩ) ──┬── C6(100nF) ──┬── D2(BAT54) ──┤ ADC
           │ BLM21│            │                │               │
           │      │            │                │               │
           │      │            ▼                ▼               ▼
           │      │          GND_ANA          GND_ANA         GND_ANA
```

**Frecuencia de corte del filtro RC:**
```
fc = 1 / (2 · π · 1kΩ · 100nF) = 1.59 kHz
```
Suficiente para eliminar ruido de 20kHz del PWM y armónicos de
la hidráulica, manteniendo la respuesta del pedal (<50ms).

**D2, D3 — Diodo Schottky BAT54:**
- Clamping a +3.3V + Vf(0.3V) ≈ 3.6V
- Protege el ADC del Nano contra sobrevoltajes (hasta ±30V)
- Respuesta <1ns

**F4, F5 — Ferrite Bead BLM21PG331SN1:**
- 330Ω @ 100MHz
- Elimina ruido RF de los cables largos de los potenciómetros

### 4.2 Routing de Señales Analógicas

| Regla | Descripción |
|-------|-------------|
| **Longitud** | Cables <30cm desde sensores a PCB. Trenzados o apantallados. |
| **Guard ring** | Rodeá las trazas analógicas con GND_ANA. Anillo de 2mm mínimo. |
| **Separación** | Trazas analógicas a >5mm de trazas PWM (20kHz). |
| **Vía shielding** | Vías GND_ANA cada 5mm a lo largo de trazas analógicas. |
| **Conexión GND** | GND_ANA conectado a GND_PWR en UN solo punto (star ground). |

---

## 5. Filtrado EMI

### 5.1 Filtrado de Línea de Potencia (24V)

```
  24V_IN ──┬── F1(PTC) ── L(CM choke) ──┬── Cx ──┬── TVS ──┬── Regulador
           │             100µH           │   1µF   │          │
           │                             │   X7R   │          │
           │                             │         │          │
  24V_RTN ─┴─────────────────────────────┴─────────┴──────────┘
```

**Recomendaciones EMC para el conector de 24V:**
- Usar conector estilo **Deutsch DT06-2S** (automotriz, sellado IP67)
- Capacitor X7R 1µF entre líneas (Cx) justo en el conector
- Ferrite bead en el cable de entrada (Clip-on ferrite 3.5mm Ø)

### 5.2 Filtrado de Cable de Motor

```
  BTS7960 ──┬── FB 600Ω ──┬── J1_MOTOR
            │              │
            │              └── C_snub ──┐
            │                          │
            └──────────────────────────┴── GND_PLANE
```

**Recomendaciones EMC para el conector de motor:**
- Conector **Molex MX150** sellado (IP67)
- Ferrite bead en SMD: BLM31PG601SN1 (600Ω, 2A)
- Cable apantallado con blindaje conectado a GND_PWR en un extremo

### 5.3 Filtrado de Señales (Potenciometros)

```
  Sensor ──┬─── FB 330Ω ──── Filtro RC ──── Diodo Clamp ──── ADC
           │
           └── C_bypass(1nF) ──── GND_ANA  (justo en el conector)
```

**Conector de sensores (J2):**
- **Molex Nano-Fit** o **JST XH** de 4 pines
- Pines: 1-VCC, 2-SIG, 3-SHIELD, 4-GND
- Shield (blindaje) conectado a GND_ANA, NO a GND_PWR

### 5.4 TVS por Cada Conector que Sale de la PCB

| Conector | TVS | Ubicación |
|----------|-----|-----------|
| 24V_IN | SMCJ28CA (bidireccional) | Entre 24V y GND, justo en bornera |
| MOTOR | SMAJ18A (unidireccional) | Entre cada salida y GND |
| SENSORES | PESD5V0S1UB (ESD) | Entre SIG y GND_ANA |

---

## 6. Layout de PCB

### 6.1 Stackup Recomendado (4 capas)

| Capa | Señal | Propósito |
|------|-------|-----------|
| **Top** | Potencia + señal | Componentes de potencia, BTS7960, borneras |
| **Inner 1** | GND_PWR | Plano de tierra de potencia (24V + 5V motor) |
| **Inner 2** | GND_ANA | Plano de tierra analógica (sensores, ADC) |
| **Bottom** | Señal + 3.3V | Ruteo fino, regulador 3.3V, ferrites |

**2 capas (económico):** Separar la PCB en dos zonas claras:
- Zona de potencia (mitad superior): GND_PWR
- Zona analógica (mitad inferior): GND_ANA
- Conectar con ferrite bead (BLM21BD102SN1)

### 6.2 Separación de Planos de Tierra

```
  ┌─────────────────────────────────────────────────────┐
  │   ZONA DE POTENCIA (GND_PWR)                        │
  │   ┌───┐  ┌───────────┐  ┌───┐                       │
  │   │F1 │  │ BTS7960   │  │C1 │                       │
  │   │   │  │           │  │   │                       │
  │   └───┘  └───────────┘  └───┘                       │
  │                                                      │
  │        ═══ FB(f) ═══   (puente de ferrite)          │
  │                                                      │
  │   ZONA ANALOGICA (GND_ANA)                           │
  │   ┌───┐  ┌───────────┐  ┌───┐                       │
  │   │R2 │  │ Arduino   │  │J2 │                       │
  │   │C6 │  │ Nano      │  │   │                       │
  │   └───┘  └───────────┘  └───┘                       │
  └─────────────────────────────────────────────────────┘
```

### 6.3 Reglas de Ruteo por Tipo de Señal

| Tipo de Señal | Ancho Mínimo | Separación | Distancia a Borde |
|--------------|-------------|------------|-------------------|
| 24V_PWR | 2mm (80mil) | 3mm de otras | 5mm |
| GND_PWR | 3mm (120mil) plano | — | 3mm |
| PWM_OUT (motor) | 1.5mm (60mil) | 3mm de analógicas | 3mm |
| Señal analógica | 0.5mm (20mil) | 5mm de PWM | 2mm |
| +3.3V sensores | 0.5mm (20mil) | 1mm | 2mm |

### 6.4 Posicionamiento de Componentes

```
  ┌────────────────────────────────────────────┐
  │  J1 (24V_IN)                               │
  │  [F1] [D1] [C1] [C2] [L1] [C3] [C4]       │  <── Filtrado de entrada
  │                                            │
  │  [U1 7805 + disipador]                     │  <── Regulador 24V→5V
  │       │                                    │
  │  [C4A] [U2 AMS1117] [C5A] [C5B]           │  <── 3.3V sensores
  │       │                                    │
  │  ─────┴──── Separación física (3mm) ──────│  <── Corte en plano GND
  │       │                                    │
  │  [F4] [R2] [C6] [D2] ───── [Nano A0]     │  <── Acondicionamiento
  │  [F5] [R3] [C7] [D3] ───── [Nano A1]     │  <── canales analógicos
  │                                            │
  │  ─────┴──── Separación física (5mm) ──────│  <── Aislamiento ruido PWM
  │       │                                    │
  │  [R1] [C5] ─────────────── [Nano A2]      │  <── Sensing corriente
  │                                            │
  │  ─────┴──── Separación física (3mm) ──────│
  │       │                                    │
  │  [R4] [C10] [C11] ────── [Nano VIN]       │  <── Decoupling Nano
  │       │                                    │
  │  [SW1] [R5] ──────────── [Nano RESET]     │  <── Reset externo
  │       │                                    │
  │  ─────┴────────────────────────────────── │
  │  [BTS7960] con disipador                   │  <── Etapa de potencia
  │  [F2] [F3] [R_snub] [C_snub]             │  <── Snubber output
  │                                            │
  │  J2 (MOTOR_OUT)  J3 (SENSORES)            │  <── Conectores de salida
  └────────────────────────────────────────────┘
```

### 6.5 Recomendaciones Críticas de Layout

1. **Plano GND_PWR sin interrupciones** debajo del BTS7960 — usar vías
   térmicas para disipación.

2. **Nunca rutear señales analógicas paralelas a trazas PWM** por más
   de 5mm. Si es inevitable, usar vía GND entre ellas.

3. **Capacitor de desacople C6 (100nF) lo más cerca posible del pin A0
   del Nano.** El lazo de corriente del ADC debe ser mínimo.

4. **Stencil de soldadura para el BTS7960** — el pad térmico central
   requiere soldadura uniforme. Usar 9 vías térmicas al plano GND.

5. **Disipador para 7805** obligatorio. A 24V con 200mA, disipa ~3.8W.
   Usar disipador TO-220 de 20°C/W o mejor.

6. **Separación galvánica**: si los sensores están a >1m de distancia,
   agregar optoacopladores (ISO7240) en las líneas de señal. Para este
   diseño (cables <30cm), no es necesario.

---

## 7. Lista de Componentes

### 7.1 Protección y Filtrado de Entrada

| Ref | Componente | Especificación | Cant. | Notas |
|-----|-----------|---------------|-------|-------|
| F1 | PTC reseteable | 3A, 30V, RXEF030 | 1 | Protección sobrecorriente |
| D1 | TVS bidireccional | SMCJ28CA, SMC | 1 | Load dump ISO 7637-2 |
| L1 | Common mode choke | 100µH, 2A, TH | 1 | Würth 744823310 |
| C1 | Electrolítico | 470µF/50V, 105°C | 1 | Panasonic FR |
| C2 | MLCC X7R | 100nF/50V, 1206 | 1 | — |
| C3 | Electrolítico | 100µF/35V, 105°C | 1 | — |
| C4 | MLCC X7R | 100nF/50V, 1206 | 1 | — |

### 7.2 Regulación

| Ref | Componente | Especificación | Cant. | Notas |
|-----|-----------|---------------|-------|-------|
| U1 | Regulador lineal | LM2940T-5.0, TO-220 | 1 | Automotive, low dropout |
| U2 | Regulador 3.3V | AMS1117-3.3, SOT-223 | 1 | — |
| C4A | Tantalio | 10µF/16V, SMD | 1 | Salida 7805 |
| C5A | Tantalio | 10µF/10V, SMD | 1 | Salida AMS1117 |
| C5B | MLCC X7R | 100nF/16V, 0805 | 1 | HF decoupling 3.3V |

### 7.3 Acondicionamiento Analógico

| Ref | Componente | Especificación | Cant. | Notas |
|-----|-----------|---------------|-------|-------|
| F4, F5 | Ferrite bead | BLM21PG331SN1, 0805 | 2 | 330Ω @ 100MHz |
| R2, R3 | Resistor | 1kΩ ±1%, 0805 | 2 | Filtro RC |
| C6, C7 | MLCC NP0/C0G | 100nF/50V, 0805 | 2 | Filtro RC (baja deriva) |
| D2, D3 | Schottky diode | BAT54, SOT-23 | 2 | Clamp ADC |

### 7.4 Etapa de Potencia

| Ref | Componente | Especificación | Cant. | Notas |
|-----|-----------|---------------|-------|-------|
| U3 | Puente H | BTS7960B, TO-263 | 1 | Con disipador |
| R1 | Resistor sense | 1kΩ ±1%, 0805 | 1 | IS_SENSE |
| C5 | MLCC X7R | 100nF/50V, 0805 | 1 | Filtro IS_SENSE |
| F2, F3 | Ferrite bead | BLM31PG601SN1, 1206 | 2 | 600Ω @ 100MHz, 2A |
| R_snub | Resistor snubber | 10Ω 1W, axial | 2 | Carbon composition |
| C_snub | MLCC NP0 | 1nF/1kV, 1206 | 2 | Snubber output |

### 7.5 Decoupling y Misceláneos

| Ref | Componente | Especificación | Cant. | Notas |
|-----|-----------|---------------|-------|-------|
| R4 | Resistor | 10Ω 1/4W, 1206 | 1 | Aislar VIN Nano |
| C10 | Electrolítico | 100µF/16V | 1 | Bulk decoupling Nano |
| C11 | MLCC X7R | 100nF/50V, 0805 | 1 | HF decoupling Nano |
| C8, C9 | MLCC X7R | 100nF/50V, 0805 | 2 | Decoupling sensores |
| SW1 | Tact switch | 6x6mm, TH | 1 | Reset externo |
| R5 | Resistor pull-up | 10kΩ ±5%, 0805 | 1 | Pull-up RESET |

### 7.6 Conectores

| Ref | Tipo | Especificación | Cant. | Notas |
|-----|------|---------------|-------|-------|
| J1 | Bornera 2 pines | Deutsch DT06-2S | 1 | 24V entrada |
| J2 | Header 4 pines | Molex Nano-Fit 4pos | 1 | Sensores |
| J3 | Bornera 2 pines | Molex MX150 2pos | 1 | Motor |

---

## 8. Procedimiento de Validación

### 8.1 Pruebas de Laboratorio

| Test | Equipo | Criterio | ISO |
|------|--------|----------|-----|
| Ripple 24V entrada | Osciloscopio + sonda diferencial | <100mVpp | 7637-2 |
| Ripple +5V | Osciloscopio 20MHz BW | <50mVpp | — |
| Ripple +3.3V | Osciloscopio 20MHz BW | <30mVpp | — |
| Ruido ADC (A0 en reposo) | Multímetro True RMS | <2mVrms | — |
| Ruido ADC con motor PWM | Osciloscopio AC coupled | <5mVrms | 13766 |
| Transiente load dump | Generador de pulsos ISO 7637-2 | Sin latch-up ni reset | 7637-2 |
| Radiated emissions | Antena cerca de cable motor | <40dBµV/m | 13766 |

### 8.2 Pruebas en Máquina (Excavadora Hyundai R250 LC7)

| Test | Procedimiento | Esperado |
|------|-------------|----------|
| Arranque motor diesel | Operar normal con motor diesel al ralentí | Sin glitches en pedal |
| Carga hidráulica máxima | Accionar cilindros a máxima presión | Sin oscilaciones en setpoint |
| Sobrecorriente simulada | Trabar motor acelerador manualmente | Fault detectado en <50ms |
| Reset con switch | Presionar SW1 durante operación normal | Vuelve a operar sin recalibrar |

### 8.3 Checklist de Diseño Antes de Enviar a Fabricar

- [ ] TVS en entrada 24V (SMCJ28CA) — verificar polaridad
- [ ] Snubber RC en salidas de motor
- [ ] Ferrite beads en cada conector que sale de la PCB
- [ ] Planos GND_PWR y GND_ANA separados con puente ferrite
- [ ] Diodos BAT54 en entradas ADC
- [ ] Capacitores de desacople (100nF) en cada pin VCC del Nano
- [ ] Disipador para 7805 calculado (3.8W mínimo)
- [ ] Switch de reset con pull-up externa
- [ ] Separación de 3mm entre zona de potencia y analógica
- [ ] Vías de alivio térmico en pad del BTS7960 (9 vías)

---

## Apéndice: Cálculo de Disipación del 7805

```
V_in = 24V (nominal) / 36V (máximo con batería en carga)
V_out = 5V
I_max = 200mA (Nano + BTS7960 lógica + sensores)

P_dis = (V_in - V_out) · I_max

Nominal:  (24 - 5) · 0.2 = 3.8W
Máximo:   (36 - 5) · 0.2 = 6.2W

Disipador mínimo: 20°C/W para mantener T_j < 125°C a 40°C ambiente
T_j = 40 + (3.8 · 20) = 116°C ✓
```

> **Alternativa recomendada:** Reemplazar 7805 por DC-DC Step-Down
> (TPS54360 o LMR16030) para reducir la disipación a <0.5W y mejorar
> eficiencia a >85%. El esquemático se mantiene, solo cambia U1.

---

*Documento mantenido por Electronic Engineering Partner — Especializado en
Maquinaria Pesada. Revisar y validar contra normativas antes de fabricar.*
