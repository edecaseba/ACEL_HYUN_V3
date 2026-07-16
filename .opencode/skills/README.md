# Skill: validate_orchestration

## Propósito
Valida la orquestación completa de agentes: archivos, permisos, alineación y fallback.

## Uso
```bash
# Validar todo el sistema
python .opencode/skills/validate_orchestration.py

# Validar solo archivos críticos
python .opencode/skills/validate_orchestration.py files

# Validar solo permisos
python .opencode/skills/validate_orchestration.py permissions

# Validar solo fallback
python .opencode/skills/validate_orchestration.py fallback

# Validar solo alineación
python .opencode/skills/validate_orchestration.py alignment
```

## Resultados
El script retorna un estado con:
- **status**: "PASS" o "FAIL"
- **findings**: Lista de hallazgos
- **errors**: Lista de errores críticos
- **warnings**: Lista de advertencias

## Validaciones incluidas

### Archivos críticos
- `ai/hardware_target.json` - Configuración de hardware
- `AGENTS.md` - Reglas globales del equipo
- `.opencode/agents/orchestrator.md` - Flujo de autorización
- `.opencode/agents/explore.md` - Agente de exploración

### Permisos de agentes
- Verifica que todos los agentes tengan permisos de lectura
- Valida configuración de permisos de bash

### Fallback
- Verifica existencia de `orchestrator_fallback`
- Verifica existencia de `planner_fallback`
- Valida prioridad de fallback

### Alineación
- Verifica que AGENTS.md contenga protocolo de autorización
- Verifica que opencode.json tenga al menos 8 agentes
- Valida consistencia entre archivos

## Ejemplo de salida
```
=== VALIDACIÓN DE ORQUESTACIÓN ===
Estado: PASS

Hallazgos:
  ✅ Validación completada exitosamente
  ⚠️ 2 advertencias encontradas

Advertencias:
  ⚠️ orchestrator_fallback no encontrado en opencode.json
  ⚠️ Solo 7 agentes definidos, se esperan al menos 8
```