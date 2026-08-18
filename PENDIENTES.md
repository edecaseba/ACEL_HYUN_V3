# PENDIENTES — ACEL_HYUN_V3

**Última actualización:** 2026-08-17
**Estado del repo:** todo comiteado y pusheado a `origin/main` (Gitea). Working tree limpio.

---

## ✅ Hecho hoy (2026-08-17)

Tres commits en `main`, todos con build (`pio run -e nanoatmega328`) y tests (`pio test -e native`) en verde:

1. **`a275fda`** — Migración del pipeline de agentes de opencode a Claude Code nativo (`.claude/agents/`, `.claude/skills/`, `CLAUDE.md`) + fix crítico de firmware: el motor reseteaba el micro al arrancar. Causa raíz: PWM del puente H a 62.5kHz (por encima del límite real del IBT-2/BTS7960, 25kHz) + bug de modo Timer1 (WGM 15 en vez de 14) + colisión de direcciones EEPROM entre `Config` y la calibración de sobrecorriente. v2.0.24.
2. **`b65be96`** — `README.md` y `CALIBRACION_GUIA.md` actualizados (habían quedado en v2.0.21, sin documentar `ACAL`).
3. **`6a52b29`** — Detección de pérdida de señal (`checkSignalLoss()` / `src/signal_loss.h`): el actuador **no tiene finales de carrera físicos**, así que una lectura de pedal/feedback fuera del rango calibrado ahora dispara Safe State. v2.0.25.

Decisión tomada y documentada (`CLAUDE.md`, `ai/hardware_target.json`): **sin migración a Rust ni a ESP32-S3, nunca** — la placa ya está fabricada con el ATmega328P y no es seguro migrar el control de este motor a un ecosistema menos maduro en un MCU de esta clase.

---

## ⚠️ CRÍTICO — nada de esto se probó en hardware real todavía

Todo lo de hoy se validó con `pio run` (compila) y `pio test -e native` (tests unitarios con mocks). **Ningún cambio se subió ni se probó en el Arduino Nano real conectado al IBT-2 y al actuador de la excavadora.** Antes de dar por cerrado el fix del reset:

1. Flashear el firmware (`pio run -e nanoatmega328 -t upload`).
2. Abrir el monitor serie y confirmar el mensaje `Reset cause (MCUSR)` al bootear — debería decir `POWER-ON` en un arranque normal.
3. Con el osciloscopio, confirmar que el PWM en D9/D10 conmuta a ~20kHz (no a 62.5kHz ni a otra frecuencia) cuando el motor se mueve.
4. Recalibrar de cero (`CAL` o `ACAL`) — cualquier calibración guardada en EEPROM antes de hoy pudo haberse hecho con el firmware roto (PWM 62.5kHz) y no es confiable. Ver `CALIBRACION_GUIA.md`.
5. Confirmar que el motor arranca sin resetear el micro, con el pedal en distintas posiciones (no solo en ralentí).
6. Probar la detección de pérdida de señal (`checkSignalLoss`) a propósito: desconectar el potenciómetro de feedback con el motor en operación normal y confirmar que dispara Safe State en <100ms sin falsos positivos durante operación normal (vibración de la máquina, bordes del rango calibrado). El margen actual es 50 cuentas ADC / 50ms — si da falsos positivos en la máquina real, ajustar `SIGNAL_LOSS_MARGIN_ADC` en `src/signal_loss.h` (no en `main.cpp`, quedó centralizado ahí).

## Nota sobre EEPROM existente
La calibración de sobrecorriente ahora vive en addr 64+ (antes colisionaba con `Config` en addr 0). Si el Arduino ya tenía una calibración de sobrecorriente guardada de antes de hoy, esa dirección vieja (addr 0-3) ya no se lee — el firmware va a ver la región nueva (addr 64+) vacía (`0xFFFF`) y va a recalibrar sobrecorriente solo al bootear (comportamiento esperado, ver `setup()`). No hace falta hacer nada manual para esto, pero confirmarlo en el primer boot real.

---

## Referencias rápidas
- Diagnóstico completo de la falla de reset: `.claude/skills/motor-startup-diagnostics/SKILL.md`
- Specs del IBT-2/BTS7960: `.claude/skills/ibt2-bts7960/SKILL.md`
- Reglas MISRA/ISO/Safe State: `.claude/skills/embedded-safety-rules/SKILL.md`
- Hardware único (pines, memoria, target): `ai/hardware_target.json`
- Procedimiento de calibración (manual y `ACAL`): `CALIBRACION_GUIA.md`
- Historial técnico versión a versión: `CHANGELOG.md`
