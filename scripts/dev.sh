#!/usr/bin/env bash
# Lanza backend (FastAPI con venv activado) y frontend (Vite) en paralelo.
# Uso: ./scripts/dev.sh

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Activador del venv segun SO (Git Bash en Windows usa Scripts/)
if [[ -f "$repo_root/backend/venv/bin/activate" ]]; then
    venv_activate="$repo_root/backend/venv/bin/activate"
elif [[ -f "$repo_root/backend/venv/Scripts/activate" ]]; then
    venv_activate="$repo_root/backend/venv/Scripts/activate"
else
    echo "[dev] No se encontro el venv en backend/venv."
    exit 1
fi

backend_pid=""
frontend_pid=""

cleanup() {
    echo ""
    echo "[dev] Deteniendo procesos..."
    [[ -n "$backend_pid"  ]] && kill "$backend_pid"  2>/dev/null || true
    [[ -n "$frontend_pid" ]] && kill "$frontend_pid" 2>/dev/null || true
    wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "[dev] Backend  -> http://127.0.0.1:8000"
(
    cd "$repo_root/backend"
    # shellcheck disable=SC1090
    source "$venv_activate"
    fastapi dev app/main.py
) &
backend_pid=$!

echo "[dev] Frontend -> http://127.0.0.1:5173"
(cd "$repo_root/frontend" && npm run dev) &
frontend_pid=$!

echo "[dev] Ctrl+C para detener ambos."
wait -n
