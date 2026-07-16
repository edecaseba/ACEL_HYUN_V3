# AGENTS.md — Reglas Globales del Equipo

> Contexto del hardware en `ai/hardware_target.json`. Persona en `ai/persona.md`.
> Reglas MISRA/ISO/IBT-2 en `ai/code_rules.md` + skills bajo demanda.
> **NO releer archivos** — ya están inyectados.

## Pipeline (post-aprobación del plan)
1. @fw-planner: diseña arquitectura y plan de implementación.
2. @fw-coder: implementa, compila, lanza @fw-reviewer via `task`.
3. @fw-reviewer: audita → **APROBADO** o **RECHAZADO** (veto absoluto).
4. @fw-tester: genera tests Unity + TEST_PROCEDURE.md, lanza @fw-documenter.
5. @fw-documenter: actualiza CHANGELOG.md, reporta resumen.

> Si @fw-reviewer rechaza → pipeline se detiene, @fw-coder corrige y reintenta.
> El meta‑orquestador (tú) orquesta el flujo y gestiona reintentos por cuota.

## Permisos
| Agente       | File | Bash |
|--------------|------|------|
| fw-planner   | R    | ❌   |
| fw-coder     | R/W  | ✅   |
| fw-reviewer  | R    | ❌   |
| fw-tester    | R/W  | ✅   |
| fw-documenter| R/W  | ❌   |
| fw-explorer  | R    | ❌   |

## Fallback por cuota (429)
Cualquier agente falla → siguiente en cadena (alterna modelos):
@fw-planner(nvidia/nvidia/nemotron-3-ultra-550b-a55b) → @fw-coder(nvidia/nvidia/nemotron-3-ultra-550b-a55b) → @fw-reviewer(nvidia/nvidia/nemotron-3-ultra-550b-a55b) → @fw-tester(nvidia/nvidia/nemotron-3-ultra-550b-a55b) → @fw-documenter(nvidia/nvidia/nemotron-3-nano-30b-a3b) → @fw-explorer(nvidia/nvidia/nemotron-3-nano-30b-a3b)

Si todo el pool agotado → notificar y detener.

## Agentes adicionales (uso bajo demanda)
| Agente         | Modelo                                  | Uso                          |
|----------------|-----------------------------------------|------------------------------|
| fw-build       | nvidia/nvidia/nemotron-3-nano-30b-a3b   | Compilar y generar artefactos |
| fw-compactor   | nvidia/nvidia/nemotron-3-nano-30b-a3b   | Optimizar tamaño binario      |
| fw-general     | nvidia/nvidia/nemotron-3-nano-30b-a3b   | Tareas de propósito general   |
| fw-plan        | nvidia/nvidia/nemotron-3-nano-30b-a3b   | Generar/mantener planes       |
| fw-summary     | nvidia/nvidia/nemotron-3-nano-30b-a3b   | Resúmenes ejecutivos          |
| fw-titler      | nvidia/nvidia/nemotron-3-nano-30b-a3b   | Títulos/encabezados docs      |
| calibrator     | nvidia/nvidia/nemotron-3-ultra-550b-a55b | Calibración sensores/actuadores |
| slicer         | nvidia/nvidia/nemotron-3-nano-30b-a3b   | Preparación archivos fabricación |

## Regla estricta: Pipeline obligatorio
**Toda modificación, corrección, feature, bugfix o cambio en el proyecto DEBE ejecutarse a través del pipeline de agentes dedicado.**

- Prohibido: cambios directos por el meta-orquestador sin pasar por @fw-planner → @fw-coder → @fw-reviewer → @fw-tester → @fw-documenter
- Excepción única: emergencias de hardware crítico (motor no gira, safe-state falla) donde el meta-orquestador aplica hotfix y **luego** documenta via pipeline
- El meta-orquestador SOLO orquesta: lanza agents via `task`, valida outputs, gestiona fallbacks

## Reglas
- ✅ Archivos completos, nunca fragmentos
- ✅ Seguridad funcional: detener si viola ISO/MISRA
- ✅ Zero Dynamic RAM post-init
- ✅ Tests Unity obligatorios
- ✅ Castellano, tono directo, sin saludos