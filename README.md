# CLI de Transformaciones de Imagen

Este repositorio incluye un CLI simple para ejecutar las 6 transformaciones del assignment.

Por ahora, el CLI **no incluye** el parametro de threads (6, 12, 18). Solo prepara el flujo de compilacion/ejecucion.

## Scripts disponibles

- `run_transform` -> wrapper multiplataforma (recomendado)
- `run_transform.sh` -> macOS/Linux
- `run_transform.ps1` -> Windows PowerShell

## Requisitos

- Compilador C en PATH: `gcc` o `clang`
- Para Windows: `pwsh` (PowerShell 7+) o `powershell`

## Operaciones

- `ihg`: inversion horizontal gris
- `ivg`: inversion vertical gris
- `dkg`: desenfoque kernel gris
- `ihc`: inversion horizontal color
- `ivc`: inversion vertical color
- `dkc`: desenfoque kernel color

## Uso rapido

### macOS/Linux

```bash
chmod +x run_transform run_transform.sh
./run_transform list
./run_transform build all
./run_transform run ihg ./imagenes/entrada.bmp ./salidas/ihg_salida.bmp
./run_transform run-all ./imagenes/entrada.bmp ./salidas
```

### Windows PowerShell

```powershell
./run_transform.ps1 list
./run_transform.ps1 build all
./run_transform.ps1 run ihg .\imagenes\entrada.bmp .\salidas\ihg_salida.bmp
./run_transform.ps1 run-all .\imagenes\entrada.bmp .\salidas
```

## Notas

- La interfaz esperada para cada binario es: `<input.bmp> <output.bmp>`.
- Si el binario no existe, el CLI intenta compilar automaticamente.
- Si tus archivos `.c` aun no tienen implementacion (`main`), la compilacion/ejecucion fallara hasta completarlos.

## Levantar el monorepo (backend + frontend)

Scripts en `scripts/` que activan el venv del backend, lanzan `fastapi dev` y `npm run dev` en paralelo.

- `scripts/dev.ps1` -> Windows PowerShell
- `scripts/dev.sh`  -> macOS/Linux (y Git Bash en Windows)

> Requisito: el venv del backend y `node_modules` del frontend deben estar instalados previamente.

Uso:

```powershell
# Windows
./scripts/dev.ps1
```

```bash
# macOS/Linux
./scripts/dev.sh
```

- Backend  -> http://127.0.0.1:8000
- Frontend -> http://127.0.0.1:5173
- `Ctrl+C` detiene ambos procesos.
