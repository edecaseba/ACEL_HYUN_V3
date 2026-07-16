# PENDIENTES - ACEL_HYUN_V3 Pipeline
**Fecha:** 2026-07-15
**Estado:** Código validado (30/30 tests, compilación OK) - Pipeline bloqueado por configuración opencode

---

## ✅ COMPLETADO

### Código Firmware (validado)
- `src/overcurrent.h` - `#define` → `constexpr` + `OC_MIN_THRESHOLD = 950`
- `src/overcurrent.cpp` - `static_cast` explícitos en todos los casts
- `src/main.cpp`:
  - PWM 20kHz (Timer1 Fast PWM 10-bit, ICR1=799)
  - EMA alpha corregidos: 0.75/0.80/0.85 (fc ≈ 1.5 kHz)
  - `delayMicroseconds(100)` eliminado del `loop()`
  - Range-based for → for clásico (MISRA C++:2008)
  - `ucase()` narrowing fix: `*s = static_cast<char>(*s - ('a' - 'A'))`
- **Tests Unity: 30/30 PASS** (`pio test -e native`)
- **Compilación nanoatmega328: SUCCESS** - RAM 27.1%, Flash 48.8%

### Configuración
- `opencode.json` simplificado (sin sección `permission` problemática)
- `AGENTS.md` actualizado con fallback chain NVIDIA
- `.opencode/agents/*.md` actualizados (15 archivos)

---

## ❌ PENDIENTES CRÍTICOS

### 1. Corregir nombres de modelos en `.opencode/agents/*.md`
**Problema:** Los logs muestran `ProviderModelNotFoundError: Model not found: nvidia/nemotron-3-ultra-550b-a55b`
**Modelo correcto:** `nvidia/nvidia/nemotron-3-ultra-550b-a55b` (doble `nvidia/`)

**Archivos a corregir (15):**
```
.opencode/agents/fw-planner.md
.opencode/agents/fw-coder.md
.opencode/agents/fw-reviewer.md
.opencode/agents/fw-tester.md
.opencode/agents/fw-documenter.md
.opencode/agents/fw-explorer.md
.opencode/agents/fw-build.md
.opencode/agents/fw-compactor.md
.opencode/agents/fw-general.md
.opencode/agents/fw-plan.md
.opencode/agents/fw-summary.md
.opencode/agents/fw-titler.md
.opencode/agents/calibrator.md
.opencode/agents/slicer.md
.opencode/agents/meta-orquestador.md (si existe)
```

**Cambio:** `nvidia/nemotron-3-ultra-550b-a55b` → `nvidia/nvidia/nemotron-3-ultra-550b-a55b`
**Cambio:** `nvidia/nemotron-3-nano-30b-a3b` → `nvidia/nvidia/nemotron-3-nano-30b-a3b`

### 2. Limpiar cache y reiniciar opencode
```bash
rm -rf ~/.config/opencode ~/.local/share/opencode/repos/ACEL_HYUN_V3* ~/.cache/opencode/*
```

### 3. Validar agentes
```bash
opencode debug agent fw-coder
opencode debug agent fw-reviewer
opencode run "test"
```

### 4. Ejecutar pipeline completo
Una vez que los agentes respondan:
1. `@fw-planner` - Diseñar arquitectura (si hay cambios nuevos)
2. `@fw-coder` - Implementar (ya hecho, pero validar)
3. `@fw-reviewer` - Auditar (ya validado manualmente)
4. `@fw-tester` - Tests (30/30 PASS confirmado)
5. `@fw-documenter` - Actualizar CHANGELOG.md

---

## 📋 LOGS DE ERROR RELEVANTES

```
ResourceExhausted: Worker local total request limit reached (33/32)
ProviderModelNotFoundError: Model not found: nvidia/nemotron-3-ultra-550b-a55b
  Did you mean: nvidia/nvidia/nemotron-3-ultra-550b-a55b?
ProviderModelNotFoundError: Model not found: meta/llama-3.3-70b-instruct
ProviderModelNotFoundError: Model not found: ibm/granite-34b-code-instruct
```

---

## 🎯 PRÓXIMA SESIÓN
1. Aplicar corrección de nombres de modelos (doble `nvidia/`)
2. Limpiar cache opencode
3. Probar `opencode run "hola"` 
4. Si funciona → ejecutar pipeline completo
5. Actualizar CHANGELOG.md a v2.0.19

---

**Nota:** El firmware está listo para producción. El bloqueo es 100% configuración de modelos en opencode.