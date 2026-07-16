---
name: validate-orchestration
description: "Valida la orquestación completa de agentes: archivos críticos existentes, permisos de agentes alineados con AGENTS.md, configuración de fallback, y coherencia entre opencode.json y los agentes .md en .opencode/agents/. Cargar esta skill cuando se requiera verificar la integridad del equipo de agentes antes o después de cambios."
license: MIT
compatibility: opencode
metadata:
  dominio: infraestructura
  fuente: "OpenCode agent system — autodescripción del proyecto"
---

## Qué hago
- Verifico que todos los archivos de agente en `.opencode/agents/` tengan frontmatter válido (description, mode, model, permissions).
- Verifico que cada agente en `opencode.json` tenga un archivo `.md` correspondiente en `.opencode/agents/`.
- Verifico que los permisos declarados en `AGENTS.md` coincidan con los permisos en `opencode.json`.
- Verifico que exista `orchestrator_fallback` configurado.
- Verifico que `ai/hardware_target.json` tenga `active_target` definido (no "definir_por_orquestador").
- Verifico que `ai/code_rules.md` exista y no esté vacío.

## Cuándo usarme
- Después de crear o modificar agentes.
- Antes de iniciar un pipeline de desarrollo.
- Cuando se reporten errores de "agent not found" o permisos denegados.

## Procedimiento
1. Listar archivos en `.opencode/agents/` y extraer nombres.
2. Leer `opencode.json` y extraer lista de agentes definidos.
3. Cruzar ambas listas: detectar agentes sin archivo `.md` y archivos `.md` sin entrada en JSON.
4. Leer `AGENTS.md` y verificar tabla de permisos contra `opencode.json`.
5. Verificar `ai/hardware_target.json` → `active_target`.
6. Reportar: PASS/FAIL con lista de discrepancias.

## Referencias
- OpenCode agent configuration: `opencode.json`
- Agent definitions: `.opencode/agents/*.md`
- Global rules: `AGENTS.md`
- Hardware target: `ai/hardware_target.json`