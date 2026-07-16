---
name: skill-iso-safety
description: Normativas ISO de seguridad industrial para maquinaria pesada. ISO 13849, ISO 7637-2, ISO 13766. Cargar solo cuando se requiera validacion normativa detallada.
license: MIT
compatibility: opencode
metadata:
  dominio: firmware
  fuente: "ISO 13849-1:2015, ISO 7637-2:2011, ISO 13766:2006"
---

# ISO Safety Norms Reference

## ISO 13849-1 (Safety of machinery)
- Safe State: ante fallo crítico (pérdida sensor, timeout, sobrecorriente), salidas potencia → 0 lógico/alta impedancia
- Boot security: configurar pines GPIO y safe-states por registro antes de activar etapas potencia
- Categoría requerida: PLr = c (performance level)

## ISO 7637-2 (Road vehicles — electrical transients)
- Pulse 1: -75V, 2ms (suppression)
- Pulse 2a: +37V, 50µs (alternator)
- Pulse 3a/3b: ±75V, 0.1µs (switching)
- Pulse 4: <5V cranking, 10ms
- Pulse 5: +87V load dump, 400ms
- Mitigación: TVS SMCJ28CA en entrada + filtrado digital EMA en ADC

## ISO 13766 (Earth-moving machinery — EMC)
- Radiated emissions: <40dBµV/m (30-1000MHz)
- Conducted emissions: <60dBµV (0.15-30MHz)
- Immunity: 30V/m (20-1000MHz), 60V/m (pulsed)
- Mitigación: PWM 20kHz, snubber RC, ferrites axiales THT, CM choke

## Criterios de aprobación (reviewer)
- [ ] TVS en entrada 24V
- [ ] Snubber RC en salidas motor
- [ ] Ferrite beads en conectores salida
- [ ] Safe State implementado
- [ ] Dead-time 150ms respetado