# BAMBU_AGENTS.md — Equipo Bambu Studio (solo modelos gratuitos)

> Contexto: `ai/bambu_target.json` — impresora A1 + AMS Lite
> Persona equipo: `ai/bambu_persona.md`

## Pipeline Bambu Studio (todos gratuitos)

```
tú (usuario) → @orquestador (deepseek-v4-flash-free) 
  → [pregunta] → @slicer (deepseek-v4-flash-free) → (vuelve)
  → [si calibración] → @calibrator (north-mini-code-free) → (vuelve)
  → [revisión] → @reviewer (mimo-v2.5-free) → (vuelve)
  → [documentación] → @documenter (mimo-v2.5-free) → (vuelve)
  → FIN → respuesta al usuario
```

## Modelos y roles

| Agente | Modelo (gratuito) | Rol en Bambu Studio | Herramientas |
|--------|-------------------|---------------------|--------------|
| **orquestador** | `opencode/deepseek-v4-flash-free` | Coordinador. Recibe tu consulta, arma el pipeline, consolida respuestas | File R/W, Bash |
| **@slicer** | `opencode/deepseek-v4-flash-free` | Ingeniero de perfiles. Genera configs de filamento/placa/AMS | File R/W, Bash |
| **@calibrator** | `opencode/north-mini-code-free` | Especialista en calibración. Genera G-code de pruebas y ajustes numéricos | File R/W, Bash |
| **@reviewer** | `opencode/mimo-v2.5-free` | Revisor. Veta configs peligrosas, verifica límites A1 y compatibilidad AMS | File R |
| **@documenter** | `opencode/mimo-v2.5-free` | Documenta perfiles creados, CHANGELOG de configuraciones | File R/W |

## Permisos por agente

| Agente | File R | File W | Bash |
|--------|--------|--------|------|
| orquestador | ✅ | ✅ | ✅ |
| @slicer | ✅ | ✅ | ✅ |
| @calibrator | ✅ | ✅ | ✅ |
| @reviewer | ✅ | ❌ | ❌ |
| @documenter | ✅ | ✅ | ❌ |

## Flujo de trabajo típico

### 1. Consulta del usuario
Dices algo como: *"Necesito perfil para PLA metalizado con AMS Lite, se adhiere mal"*

### 2. Orquestador analiza
Determina si necesitas:
- Perfil nuevo → activa @slicer
- Calibración → activa @calibrator
- Revisión de seguridad → activa @reviewer
- Documentación → activa @documenter

### 3. Ejecución del pipeline
```
orquestador → @slicer → [resultado] → orquestador
  → [si calibración necesaria] → @calibrator → orquestador
  → [seguridad] → @reviewer → orquestador
  → [registro] → @documenter → orquestador
  → respuesta final al usuario
```

### 4. Continuidad por cuota (Partial Handoff)
Si un agente falla por cuota:
- `@slicer` (deepseek) falla → releva `@calibrator` (north-mini) en modo slicer
- `@reviewer` (mimo) falla → releva `@slicer` (deepseek) en modo reviewer
- `@documenter` (mimo) falla → releva `@orquestador` directamente

## Reglas específicas Bambu Studio

1. **Límites de seguridad A1** — Nunca sugerir:
   - Nozzle > 300°C
   - Cama > 100°C
   - Volumetric flow > 32 mm³/s en hotend stock
   - Overrides que desactiven thermal runaway protection

2. **Compatibilidad AMS Lite**
   - Solo filamento 1.75mm
   - TPU flexible no recomendado en AMS (solo externo)
   - Verificar drying recomendado antes de perfil

3. **Formato de salida**
   - Perfiles: JSON listo para importar en Bambu Studio
   - G-code calibración: .gcode listo para ejecutar
   - Documentación: Markdown en `ai/bambu_profiles/`

4. **Archivos de trabajo**
   - Los perfiles creados se almacenan en `ai/bambu_profiles/`
   - Los G-code de calibración en `ai/bambu_calibration/`
   - El CHANGELOG de configuraciones en `ai/bambu_changelog.md`
