# Configuración KiCad — ACEL_HYUN_V3

## Estructura de archivos KiCad dentro de `hardware/`

```
hardware/
├── ACEL_HYUN_V3.kicad_pro    # Proyecto KiCad
├── ACEL_HYUN_V3.kicad_sch    # Esquemático
├── ACEL_HYUN_V3.kicad_pcb    # PCB layout
├── BOM.csv                   # Bill of Materials (componentes)
├── PCB_DESIGN_GUIDE.md        # Esta guía
├── KiCAD_SETUP.md             # Este archivo
├── lib/                       # Librerías personalizadas
│   ├── ACEL_HYUN_V3.lib       # Símbolos esquemáticos
│   └── ACEL_HYUN_V3.pretty    # Footprints PCB
├── netlist/                   # Archivos de netlist
│   └── ACEL_HYUN_V3.net
└── gerber/                    # Outputs de fabricación (en .gitignore)
```

## Cómo iniciar el proyecto en KiCad

1. Abrir KiCad → New Project → `hardware/ACEL_HYUN_V3`
2. Agregar librerías de componentes al proyecto:
   - `lib/ACEL_HYUN_V3.lib` (símbolos personalizados)
3. Cargar el BOM como referencia: `BOM.csv`

## Reglas de diseño (DRC) para este PCB

| Parámetro | Valor | Nota |
|-----------|-------|------|
| Capas | 4 (recomendado) / 2 (económico) | Stackup: SIG-GND-PWR-GND_ANA-SIG |
| Ancho mínimo traza | 0.25mm (10mil) | Señales analógicas |
| Ancho traza potencia | 2mm (80mil) | 24V y PWM motor |
| Clearance mínimo | 0.3mm (12mil) | General |
| Clearance 24V-GND | 3mm (120mil) | Seguridad eléctrica |
| Ancho plano GND | 3mm mínimo | Plano continuo sin interrupciones |
| Tamaño vía potencia | 0.8mm taladro / 1.6mm anillo | Para BTS7960 y 24V |
| Tamaño vía señal | 0.3mm taladro / 0.6mm anillo | Señales analógicas |
| Máscara solder | 0.1mm expansión | General |

## Netlist de Conexiones Esenciales

```
# ACEL_HYUN_V3 — Netlist de conexiones críticas
# (Compatible con KiCad y otras EDA)

# --- POTENCIA 24V ---
(24V_IN (J1-1)(F1-1))
(24V_GND (J1-2)(D1-A2)(C1-NEG)(C2-NEG)(L1-2))
(24V_PROT (F1-2)(D1-A1)(C1-POS)(C2-POS)(L1-1))

# --- ALIMENTACION 5V ---
(+5V_RAW (L1-3)(C3-POS)(C4-POS)(U1-VIN))
(GND_RAW (L1-4)(C3-NEG)(C4-NEG)(U1-GND))
(+5V (U1-VOUT)(C4A-POS)(U2-VIN)(R4-1)(C8-POS)(C9-POS)(BTS7960-VCC)(R5-1)(SW1-1))
(GND_PWR (U1-GND)(C4A-NEG)(U2-GND)(BTS7960-GND)(C5-NEG)(C10-NEG)(C11-NEG))

# --- ALIMENTACION 3.3V (SENSORES) ---
(+3.3V (U2-VOUT)(C5A-POS)(C5B-POS)(J2-1)(J2-3))
(GND_ANA (C5A-NEG)(C5B-NEG)(C6-NEG)(C7-NEG)(D2-A2)(D3-A2))

# --- MICROCONTROLADOR (Arduino Nano) ---
(NANO-VIN (R4-2)(C10-POS)(C11-POS))
(NANO-GND (C10-NEG)(C11-NEG))
(NANO-A0 (D2-K)(C6-POS)(R2-2))
(NANO-A1 (D3-K)(C7-POS)(R3-2))
(NANO-A2 (C5-POS)(R1-2))
(NANO-D10 (BTS7960-RPWM))
(NANO-D9 (BTS7960-LPWM))
(NANO-D8 (BTS7960-EN))
(NANO-RESET (SW1-2)(R5-2))

# --- ACONDICIONAMIENTO ANALOGICO ---
(PEDAL_SIG (J2-2)(F4-1))
(PEDAL_FILT (F4-2)(R2-1))
(FEED_SIG (J2-4)(F5-1))
(FEED_FILT (F5-2)(R3-1))

# --- BTS7960 POTENCIA ---
(MOTOR_A (BTS7960-OUT1)(F2-1))
(MOTOR_A_OUT (F2-2)(R_snub_A-1)(J3-1))
(MOTOR_B (BTS7960-OUT2)(F3-1))
(MOTOR_B_OUT (F3-2)(R_snub_B-1)(J3-2))
(SNUBBER_A (R_snub_A-2)(C_snub_A-1))
(SNUBBER_B (R_snub_B-2)(C_snub_B-2))
(SNUBBER_GND (C_snub_A-2)(C_snub_B-2)(GND_PWR))
```

## Footprints recomendados para fabricación

| Componente | Footprint KiCad | Notas |
|-----------|----------------|-------|
| 7805/LM2940 | Package_TO_SOT_THT:TO-220_Vertical | Con disipador |
| AMS1117 | Package_TO_SOT_SMD:SOT-223 | — |
| BTS7960B | Package_TO_SOT_SMD:TO-263-7 | Pads térmicos |
| Resistores 0805 | Resistor_SMD:R_0805 | — |
| Capacitores 1206 | Capacitor_SMD:C_1206 | Para 1kV |
| MLCC 0805 | Capacitor_SMD:C_0805 | — |
| Electrolíticos radial | Capacitor_THT:CP_Radial_D10mm | 470µF/50V |
| Ferrite bead 0805 | Ferrite_SMD:L_0805 | BLM21 |
| Ferrite bead 1206 | Ferrite_SMD:L_1206 | BLM31 |
| TVS SMC | Diode_SMD:D_SMC | SMCJ28CA |
| Tact switch THT | Button_Switch_THT:SW_Tact_6x6mm | Omron B3F |
| Deutsch DT06-2S | Connector_JAE:JAETransferStandard_2pin | 24V input |
| Molex Nano-Fit | Connector_Molex:Molex_Nano-Fit_105308-xx04 | 4 pos |
| BAT54 | Package_TO_SOT_SMD:SOT-23 | Diodo clamp |
