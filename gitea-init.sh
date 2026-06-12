#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────
# gitea-init.sh — Crear repo en Gitea y pushear local
# ──────────────────────────────────────────────────────────
# Uso:   gitea-init.sh <nombre_repo>
#        gitea-init.sh ACEL_HYUN_V3
#
# Crea el repo en Gitea vía API, inicializa .git si no existe,
# configura remote, agrega todo, commitea y pushea a main.
#
# Variables de entorno (opcionales):
#   GITEA_URL   — default: http://100.74.184.3:3000
#   GITEA_USER  — default: seba_admin
#   GITEA_TOKEN — opcional (si no se usa, usa http sin token)
# ──────────────────────────────────────────────────────────

set -euo pipefail

# ─── Configuración ────────────────────────────────────────
GITEA_URL="${GITEA_URL:-http://100.74.184.3:3000}"
GITEA_USER="${GITEA_USER:-seba_admin}"
GITEA_TOKEN="${GITEA_TOKEN:-}"

# Colores
ROJO='\033[0;31m'
VERDE='\033[0;32m'
AMARILLO='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # Sin color

# ─── Validaciones ─────────────────────────────────────────
if [ $# -lt 1 ]; then
    echo -e "${ROJO}Error: falta el nombre del repositorio.${NC}"
    echo -e "${AMARILLO}Uso:${NC} $0 <nombre_repo>"
    exit 1
fi

REPO_NAME="$1"
REPO_DIR="$(pwd)"

# ─── 1. Verificar que el directorio existe ───────────────
if [ ! -d "$REPO_DIR" ]; then
    echo -e "${ROJO}Error: el directorio '$REPO_DIR' no existe.${NC}"
    exit 1
fi

echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${CYAN}  gitea-init.sh — Repo: ${AMARILLO}${REPO_NAME}${NC}"
echo -e "${CYAN}  Directorio: ${AMARILLO}${REPO_DIR}${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

# ─── 2. Inicializar git si no existe ─────────────────────
if [ ! -d ".git" ]; then
    echo -e "${AMARILLO}[init]${NC} Inicializando repositorio git..."
    git init
    git branch -M main
    echo -e "${VERDE}[ok]${NC} Git inicializado en rama 'main'."
else
    echo -e "${AMARILLO}[skip]${NC} Git ya inicializado."
fi

# ─── 3. Crear repo en Gitea vía API ──────────────────────
echo -e "${AMARILLO}[api]${NC} Creando repositorio '${REPO_NAME}' en ${GITEA_URL}..."

if [ -n "$GITEA_TOKEN" ]; then
    AUTH_HEADER="Authorization: token ${GITEA_TOKEN}"
else
    AUTH_HEADER=""
fi

HTTP_CODE=$(curl -s -o /tmp/gitea_response.json -w "%{http_code}" \
    ${AUTH_HEADER:+-H "$AUTH_HEADER"} \
    -H "Content-Type: application/json" \
    -d "{
        \"name\": \"${REPO_NAME}\",
        \"description\": \"Creado automaticamente por gitea-init.sh\",
        \"private\": false,
        \"auto_init\": false,
        \"default_branch\": \"main\"
    }" \
    "${GITEA_URL}/api/v1/user/repos" 2>/dev/null || true)

if [ "$HTTP_CODE" = "201" ]; then
    echo -e "${VERDE}[ok]${NC} Repositorio creado exitosamente."
elif [ "$HTTP_CODE" = "409" ]; then
    echo -e "${AMARILLO}[skip]${NC} El repositorio ya existe (HTTP 409). Continuando..."
else
    MSG=$(cat /tmp/gitea_response.json 2>/dev/null || echo "sin respuesta")
    echo -e "${ROJO}[error]${NC} HTTP ${HTTP_CODE} — ${MSG}"
    echo -e "${ROJO}¿Token inválido, URL incorrecta o Gitea caído?${NC}"
    echo -e "${AMARILLO}Si no tenés token, editar este script o usar GITEA_TOKEN.${NC}"
    exit 1
fi

# ─── 4. Configurar remote ────────────────────────────────
REMOTE_URL="${GITEA_URL}/${GITEA_USER}/${REPO_NAME}.git"

if ! git remote get-url origin 2>/dev/null; then
    git remote add origin "$REMOTE_URL"
    echo -e "${VERDE}[ok]${NC} Remote origin configurado: ${REMOTE_URL}"
else
    OLD_URL=$(git remote get-url origin)
    if [ "$OLD_URL" != "$REMOTE_URL" ]; then
        git remote set-url origin "$REMOTE_URL"
        echo -e "${AMARILLO}[update]${NC} Remote origin actualizado: ${OLD_URL} → ${REMOTE_URL}"
    else
        echo -e "${AMARILLO}[skip]${NC} Remote origin ya configurado."
    fi
fi

# ─── 5. Agregar archivos y commitear ─────────────────────
echo -e "${AMARILLO}[git]${NC} Agregando archivos..."
git add -A

if git diff --cached --quiet; then
    echo -e "${AMARILLO}[skip]${NC} No hay cambios para commitear."
else
    echo -e "${AMARILLO}[git]${NC} Creando commit..."
    git commit -m "chore: inicializacion automatica de ${REPO_NAME} $(date +%Y-%m-%d)"
    echo -e "${VERDE}[ok]${NC} Commit creado."
fi

# ─── 6. Push a Gitea ─────────────────────────────────────
echo -e "${AMARILLO}[push]${NC} Pusheando a ${REMOTE_URL}..."
if git push -u origin main; then
    echo -e "${VERDE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${VERDE}  DONE — ${REPO_NAME} pusheado exitosamente${NC}"
    echo -e "${VERDE}  ${GITEA_URL}/${GITEA_USER}/${REPO_NAME}${NC}"
    echo -e "${VERDE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
else
    echo -e "${ROJO}[error]${NC} Push falló. Verificar credenciales y conectividad."
    exit 1
fi
