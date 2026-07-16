#!/usr/bin/env bash
# ============================================
# pipeline.sh — Pipeline de automatización ACEL_HYUN_V3
# Ejecuta el flujo post-cambio: build → review → test → document
# ============================================
set -euo pipefail

STEP="${1:-all}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "=== Pipeline ACEL_HYUN_V3 ==="
echo "Paso: ${STEP}"
echo "Directorio: ${PROJECT_DIR}"
echo ""

case "${STEP}" in
    build)
        echo "[1/4] Compilando..."
        platformio run --environment nanoatmega328 2>&1
        echo "✓ Build OK"
        ;;
    review)
        echo "[2/4] Lanzando @reviewer..."
        echo "   (Ejecutar manualmente en opencode: lanzar @reviewer como subagent)"
        echo "   Pendiente: esperar APROBADO del reviewer"
        ;;
    test)
        echo "[3/4] Lanzando @tester..."
        echo "   (Ejecutar manualmente en opencode: lanzar @tester como subagent)"
        echo "   Pendiente: generar tests Unity + TEST_PROCEDURE.md"
        ;;
    document)
        echo "[4/4] Lanzando @documenter..."
        echo "   (Ejecutar manualmente en opencode: lanzar @documenter como subagent)"
        echo "   Pendiente: actualizar CHANGELOG.md"
        ;;
    all)
        echo "=== Pipeline completo ==="
        bash "$0" build
        bash "$0" review
        bash "$0" test
        bash "$0" document
        echo "=== Pipeline completo finalizado ==="
        ;;
    *)
        echo "Uso: $0 {build|review|test|document|all}"
        exit 1
        ;;
esac
