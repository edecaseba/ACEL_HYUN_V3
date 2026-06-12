# CHANGELOG — ACEL_HYUN_V3
# Controlador de Acelerador — Excavadora Hyundai R250 LC7

## ESTADO ACTUAL DEL PROYECTO
> Actualizado: 2026-06-11

**Versión activa:** 1.4.0
**Estado:** Pool universal de fallback implementado — pendiente validación en hardware
**MCU activo:** Arduino Nano (ATmega328P) · PlatformIO · Arduino Framework

### Archivos existentes:

| Archivo | Estado | Descripción |
|---------|--------|-------------|
| `src/main.cpp` | ✅ Implementado | Lógica principal completa con PID integral |
| `README.md` | ✅ Creado | Documentación del proyecto + Regla de Oro Gitea |
| `ai/hardware_target.json` | ✅ Creado | Config parametrizable de MCU (Nano / ESP32-S3) |
| `ai/arquitectura_codigo.md` | ✅ Creado | Estándar MISRA, Zero RAM, PlatformIO |
| `ai/hardware_ibt2.md` | ✅ Creado | Especificaciones IBT-2 BTS7960 |
| `ai/normativas_seguridad.md` | ✅ Creado | ISO 13849, 7637-2, 13766 |
| `ai/optimizacion_main_v1.md` | ✅ Creado | Plan de optimización ejecutado |
| `ai/persona.md` | ✅ Creado | Protocolo de interacción con agentes |
| `AGENTS.md` | ✅ Actualizado v3 | Pool universal: 7 agentes intercambiables por cuota |
| `opencode.json` | ✅ Actualizado | Eliminados agentes fallback. 7 agentes con pool universal. |
| `.opencode/agents/orchestrator.md` | ✅ Actualizado | Paso 5 con cadena model-aware: planner → coder → reviewer → explore → tester → documenter → orchestrator |
| `.opencode/agents/coder.md` | ✅ Actualizado | Pipeline con pool universal + reintentos por cuota |
| `.opencode/agents/reviewer.md` | ✅ Actualizado | Sincronizado con pool universal |
| `.opencode/agents/tester.md` | ✅ Actualizado | Acepta revisores alternativos del pool |
| `.opencode/agents/documenter.md` | ✅ Actualizado | Registra uso del pool universal |
| `.vscode/tasks.json` | ✅ Creado | 5 tareas VS Code para sincronización Gitea |
| `test/test_pid.cpp` | ✅ Implementado | Tests unitarios PID con Unity (16 tests) |

### Funcionalidades implementadas (código)
- (sin cambios, las mismas de v1.1.0)

### Funcionalidades implementadas (orquestación)
- Pool Universal de Fallback: 7 agentes intercambiables por cuota
- Cadena model-aware: alterna entre North Mini → DeepSeek → Mimo → Gemini Flash → Gemini Pro
- Script `gitea-init.sh` en PATH para automatizar push a Gitea
- 5 tareas VS Code en `.vscode/tasks.json` para sincronización

### Tests
- `test/test_pid.cpp`: 16/16 tests PASSED (Unity + ArduinoFake)

### Gitea
- Repositorio creado en `http://100.74.184.3:3000/seba_admin/ACEL_HYUN_V3`
- Push inicial completado — rama `main`

### Pendiente
- ⬜ `test/TEST_PROCEDURE.md` — procedimiento de prueba en hardware
- ⬜ Validación de parámetros PID en hardware real

### Equipo de agentes activo (pool universal)

| Agente | Modelo | Proveedor | Rol |
|--------|--------|-----------|-----|
| @orchestrator | Gemini 3.1 Pro | Antigravity | Orquestación + autorización usuario |
| @planner | North Mini Code Free | OpenCode Zen | Planificación técnica detallada |
| @coder | DeepSeek V4 Flash Free | OpenCode Zen | Programación C/C++ |
| @reviewer | Mimo V2.5 Free | OpenCode Zen | Revisión (veto absoluto) |
| @tester | North Mini Code Free | OpenCode Zen | Tests y procedimientos |
| @documenter | Mimo V2.5 Free | OpenCode Zen | Documentación y CHANGELOG |
| @explore | Gemini 3.5 Flash | Antigravity | Exploración de código |

> **Pool universal:** Cualquier agente puede tomar el rol de otro si falla por cuota (429). Cadena de prioridad: @planner → @coder → @reviewer → @explore → @tester → @documenter → @orchestrator. La cadena alterna entre modelos de diferentes proveedores para minimizar agotamiento simultáneo de tokens.

---

## [1.4.0] — 2026-06-11

### Added
- **Pool Universal de Fallback**: Se reemplazó el sistema de 4 agentes de respaldo dedicados por un pool universal donde cualquier agente puede tomar el rol de otro si falla por cuota (error 429).
- **Cadena de prioridad actualizada**: La secuencia de fallback alterna entre modelos de diferentes proveedores: North Mini Code Free → DeepSeek V4 Flash Free → Mimo V2.5 Free → Gemini 3.5 Flash → Gemini 3.1 Pro.
- **Nuevos agentes en el roster**:
  - `@tester` ahora usa `opencode/north-mini-code-free` (antes Gemini 3.5 Flash)
  - `@documenter` ahora usa `opencode/mimo-v2.5-free` (antes Gemini 3.5 Flash)
  - `@explore` incorporado con Gemini 3.5 Flash
- Mecanismo de reintento en `@coder` y `@orchestrator` para fallback automático cuando un agente del pipeline no responde por cuota.
- Sección en `AGENTS.md` documentando el pool universal, la cadena de prioridad y el mecanismo de fallback con cuadro de 7 agentes.

### Changed
- `opencode.json`: eliminados los 4 agentes `*_fallback` (`planner_fallback`, `coder_fallback`, `reviewer_fallback`, `tester_fallback`). Quedan 7 agentes con roles únicos.
- `AGENTS.md`:
  - Paso 5 actualizado con la nueva cadena de fallback del pool universal
  - Tabla de permisos reducida a 7 filas
  - Mecanismo de fallback rediseñado: cualquier agente del pool puede tomar cualquier rol
  - Cuadro de modelos actualizado con 7 agentes y modelos diferenciados
- `.opencode/agents/orchestrator.md`: paso 5 actualizado con cadena model-aware que alterna modelos (planner → coder → reviewer → explore → tester → documenter → orchestrator)
- `.opencode/agents/coder.md`: agregada sección "Pipeline con Pool Universal" con reintentos por cuota y lógica de fallback
- `.opencode/agents/tester.md`: ahora acepta revisores alternativos provenientes del pool universal
- `.opencode/agents/documenter.md`: registra metadata de uso del pool universal cuando actúa como fallback
- Cadena de prioridad del pool universal optimizada para alternar entre 4 modelos diferentes (North Mini → DeepSeek → Mimo → Gemini Flash → Gemini Pro) antes de repetir modelo, minimizando agotamiento simultáneo de tokens del mismo proveedor.
- `.opencode/agents/coder.md` y `.opencode/agents/tester.md`: pipelines actualizados con alternancia de modelos en reintentos.

### Removed
- `.opencode/agents/planner_fallback.md` — eliminado. Ya no existen agentes de respaldo dedicados; el pool universal cubre cualquier rol.

---

## [1.3.0] — 2026-06-11

### Added
- **Nueva orquestación con autorización del usuario**: Gemini solo presenta plan de alto nivel y espera confirmación antes de actuar.
- **Asignación de agentes free específicos**:
  - `@planner` ahora usa `opencode/north-mini-code-free` (antes Gemini 3.1 Pro)
  - `@reviewer` ahora usa `opencode/mimo-v2.5-free` (antes MiMo)
  - `@tester` y `@documenter` siguen con Gemini 3.5 Flash (gratuito)
- **Mecanismo de fallback por cuota de Gemini**: Si el orquestador se queda sin tokens, el rol pasa automáticamente al agente más capacitado (primero @planner, luego @coder, luego @reviewer).
- Sección en `AGENTS.md` que documenta el nuevo flujo y la gestión de autorizaciones.

### Changed
- El orquestador ya no delega inmediatamente; primero muestra el plan y pide "¿Aprobado?".
- `opencode.json`: actualizados los modelos de `planner` y `reviewer`.
- Prompts de los agentes ajustados para reflejar los nuevos roles y evitar releer archivos.

### Fixed
- Potencial agotamiento de tokens de Gemini en sesiones largas: ahora las tareas pesadas (plan detallado, revisión) las realizan modelos free.

---

## [1.2.0] — 2026-06-11 (previo, solo para contexto)

... (contenido anterior)

### Notes
- Sesión: @coder (DeepSeek V4 Flash Free) usado como fallback de @documenter/@tester por cuota
- @explore (Gemini 3.5 Flash) usado como fallback de documentación
- El pool universal fue optimizado: la cadena ahora alterna entre 4 modelos diferentes antes de repetir proveedor
