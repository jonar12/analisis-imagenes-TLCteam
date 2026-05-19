#!/usr/bin/env pwsh
# Lanza backend (FastAPI con venv activado) y frontend (Vite) en paralelo.
# Uso: ./scripts/dev.ps1

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot  = (Resolve-Path (Join-Path $scriptDir '..')).Path

$backendDir  = Join-Path $repoRoot 'backend'
$frontendDir = Join-Path $repoRoot 'frontend'
$activate    = Join-Path $backendDir 'venv\Scripts\Activate.ps1'

if (-not (Test-Path $activate)) {
    Write-Host "[dev] No se encontro el venv en backend\venv." -ForegroundColor Red
    exit 1
}

# Shell anfitrion (pwsh si esta disponible, si no powershell)
$psHost = if (Get-Command pwsh -ErrorAction SilentlyContinue) { 'pwsh' } else { 'powershell' }

Write-Host "[dev] Backend  -> http://127.0.0.1:8000" -ForegroundColor Cyan
$backend = Start-Process -FilePath $psHost `
    -ArgumentList '-NoProfile','-NoLogo','-Command',". '$activate'; fastapi dev app/main.py" `
    -WorkingDirectory $backendDir `
    -NoNewWindow -PassThru

Write-Host "[dev] Frontend -> http://127.0.0.1:5173" -ForegroundColor Cyan
$frontend = Start-Process -FilePath 'cmd.exe' `
    -ArgumentList '/c','npm','run','dev' `
    -WorkingDirectory $frontendDir `
    -NoNewWindow -PassThru

Write-Host "[dev] Ctrl+C para detener ambos." -ForegroundColor DarkGray

try {
    while (-not $backend.HasExited -and -not $frontend.HasExited) {
        Start-Sleep -Milliseconds 500
    }
} finally {
    Write-Host "`n[dev] Deteniendo procesos..." -ForegroundColor Yellow
    if (-not $backend.HasExited)  { taskkill /PID $backend.Id  /T /F 2>$null | Out-Null }
    if (-not $frontend.HasExited) { taskkill /PID $frontend.Id /T /F 2>$null | Out-Null }
}
