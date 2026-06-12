# AGENTS.md — Reglas Globales del Equipo de Firmware

> Leído por TODOS los agentes antes de cada tarea.
> El contexto específico del hardware está en `ai/hardware_target.json`.
> La persona y estilo de respuesta están en `ai/persona.md`.

---

## Entorno de Desarrollo

- **IDE:** VS Code + PlatformIO
- **Framework activo:** ver `ai/hardware_target.json` → `active_target`
- **Lenguaje:** C++20
- **Extensiones de archivo:** `.cpp` / `.h` — **NUNCA `.ino`**
- **Estructura de directorios:** definida por PlatformIO (no por el agente)
  - Código fuente: `src/` (Arduino) o `main/` (ESP-IDF)
  - Tests: `test/`
  - Configuración AI: `ai/`
  - Agentes OpenCode: `.opencode/agents/`

---

## Archivos de Contexto del Proyecto

| Archivo | Propósito |
|---------|-----------|
| `ai/hardware_target.json` | MCU activo, pines, memoria, framework, reglas de código |
| `ai/arquitectura_codigo.md` | Estándar MISRA, Zero RAM, PlatformIO build |
| `ai/hardware_ibt2.md` | IBT-2 BTS7960: dead-time, PWM, shoot-through |
| `ai/normativas_seguridad.md` | ISO 13849, ISO 7637-2, ISO 13766 |
| `ai/optimizacion_main_v1.md` | Plan de la última optimización ejecutada |
| `ai/persona.md` | Tono, idioma y protocolo de entrega |
| `CHANGELOG.md` | Estado actual + historial de versiones |

---

## Protocolo del Equipo (con autorización del usuario)

### Flujo de autorización del orquestador

1. **Presentación del plan** – El @orchestrator presenta un plan de alto nivel en español con pasos numerados.
2. **Solicitud de confirmación** – El orquestador pregunta: "¿Aprobado? Responde SÍ para continuar, o indica cambios."
3. **Continuación** – Si el usuario responde SÍ, el orquestador delega inmediatamente en @planner con el mismo plan.
4. **Ajustes** – Si el usuario pide cambios, el orquestador ajusta el plan y vuelve a preguntar.
5. **Fallback por cuota (Pool Universal)** – Si el orquestador falla por cuota (error 429), notifica al usuario y traspasa el rol al siguiente agente disponible en el pool universal. La cadena alterna entre modelos diferentes para no agotar tokens del mismo proveedor: @planner (North Mini) → @coder (DeepSeek V4) → @reviewer (Mimo V2.5) → @explore (Gemini 3.5 Flash) → @tester (North Mini) → @documenter (Mimo V2.5) → @orchestrator (Gemini 3.1 Pro, último recurso).
6. **Finalización** – El flujo continúa: @planner → @coder → @reviewer → @tester → @documenter → @explore (si es necesario).

### Permisos de agentes

| Agente | Permisos de archivo | Permisos de bash |
|--------|-------------|------------|
| @orchestrator | Lectura/Escritura | Permitido |
| @planner | Lectura | Denegado |
| @coder | Lectura/Escritura | Permitido |
| @reviewer | Lectura | Denegado |
| @tester | Lectura/Escritura | Permitido |
| @documenter | Lectura/Escritura | Denegado |
| @explore | Lectura | Denegado |

### Reglas de interacción

- **No releer archivos** – Los archivos de contexto ya están inyectados automáticamente por `opencode.json → instructions`. Releerlos causa loops infinitos.
- **Entrega completa** – Entregar siempre archivos completos, nunca fragmentos parciales.
- **Seguridad primero** – Si una propuesta compromete la seguridad funcional o viola una normativa, detener la implementación inmediatamente y proponer la alternativa compatible.
- **Idioma** – Responder exclusivamente en castellano.
- **Tono** – Directo de ingeniero a ingeniero. Omitir introducciones, saludos y conceptos básicos.

### Flujo de trabajo del equipo

1. **Orquestación** – @orchestrator coordina el flujo de trabajo y gestiona la autorización del usuario.
2. **Planificación** – @planner genera el plan técnico detallado.
3. **Programación** – @coder implementa el código según el plan.
4. **Revisión** – @reviewer revisa el código con veto absoluto.
5. **Testing** – @tester genera tests unitarios y procedimientos de prueba en hardware.
6. **Documentación** – @documenter actualiza CHANGELOG.md.
7. **Exploración** – @explore busca en el codebase cuando sea necesario.

### Pipeline Automatizado (post-cambio)

> Una vez que @planner entrega el plan y el usuario lo aprueba, el pipeline corre automáticamente sin intervención manual.

| Paso | Agente | Acción |
|------|--------|--------|
| 1 | @coder | Implementa, compila, lanza @reviewer vía `task` |
| 2 | @reviewer | Audita → **APROBADO** o **RECHAZADO** (veto absoluto) |
| 3 | @tester | Si APROBADO, genera tests Unity + TEST_PROCEDURE.md, lanza @documenter vía `task` |
| 4 | @documenter | Actualiza CHANGELOG.md, reporta resumen de la sesión |

**Reglas del pipeline:**
- Si @reviewer rechaza → el pipeline se detiene. @coder corrige y reinicia desde paso 1.
- Cada agente usa la herramienta `task` para lanzar al siguiente. No esperar intervención del usuario.
- Si un agente falla por cuota (429), se aplica el pool universal (ver sección Mecanismo de fallback).
- El script `ai/pipeline.sh` documenta los pasos y puede ejecutarse manualmente si es necesario.

### Mecanismo de fallback (Pool Universal)

- **Por cuota** – Si CUALQUIER agente falla por cuota (error 429), el rol pasa automáticamente al siguiente agente disponible en el pool universal. La cadena alterna entre modelos diferentes para minimizar agotamiento simultáneo: @planner (North Mini) → @coder (DeepSeek V4) → @reviewer (Mimo V2.5) → @explore (Gemini 3.5 Flash) → @tester (North Mini) → @documenter (Mimo V2.5) → @orchestrator (Gemini 3.1 Pro). El agente que lanzó la tarea reintenta con el siguiente de la cadena hasta encontrar uno con tokens. Si se repite un modelo (ej. North Mini en planner y tester), es porque en ese punto los otros modelos ya fallaron y se reintenta por si se liberaron tokens.
- **Por veto** – Si @reviewer rechaza el código, el flujo se detiene y @coder debe corregir.
- **Por error** – Si un agente falla por error (no cuota), detener el flujo y notificar al usuario.
- **Pool agotado** – Si todos los agentes del pool están sin tokens, detener el flujo y notificar: "Pool universal agotado. Todos los modelos sin tokens."

### Cuadro de modelos por agente

| Agente | Modelo | Proveedor | Rol |
|--------|--------|-----------|-----|
| @orchestrator | Gemini 3.1 Pro | Antigravity | Orquestación + autorización |
| @planner | North Mini Code Free | OpenCode Zen | Planificación detallada |
| @coder | DeepSeek V4 Flash Free | OpenCode Zen | Programación C/C++ |
| @reviewer | Mimo V2.5 Free | OpenCode Zen | Revisión (veto absoluto) |
| @tester | North Mini Code Free | OpenCode Zen | Tests y procedimientos |
| @documenter | Mimo V2.5 Free | OpenCode Zen | Documentación y CHANGELOG |
| @explore | Gemini 3.5 Flash | Antigravity | Exploración de código |

> **Pool universal:** Cuando un agente falla por cuota, cualquier agente del pool puede tomar su rol. No existen agentes de respaldo dedicados. La cadena de prioridad alterna entre modelos de diferentes proveedores para evitar agotamiento simultáneo de tokens del mismo modelo. Los 7 agentes son intercambiables.

### Control de calidad

- **Tests unitarios** – Obligatorios para cada módulo, usando Unity.
- **Procedimientos hardware** – Obligatorios para validación en hardware real.
- **Revisión de seguridad** – Obligatoria contra ISO 13849, ISO 7637-2 e ISO 13766.
- **Verificación de memoria** – Obligatoria contra política Zero Dynamic RAM.

### Actualizaciones

- Mantener AGENTS.md sincronizado con opencode.json.
- Actualizar permisos cuando sea necesario.
- Revisar el orden de fallback regularmente.
- Documentar cualquier cambio en el protocolo de autorización.
