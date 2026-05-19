# Backend (FastAPI)

API en FastAPI para procesar imagenes. Esta guia explica como preparar el entorno virtual (`venv`) e instalar las dependencias de `requirements.txt`.

## Requisitos

- Python 3.10+ instalado y disponible en el `PATH`.
- `pip` (viene con Python).

Verifica tu version:

```bash
python --version
```

## 1. Crear el entorno virtual

Desde la carpeta `backend/`:

### Windows (PowerShell o CMD)

```powershell
python -m venv venv
```

### macOS / Linux

```bash
python3 -m venv venv
```

Esto crea la carpeta `backend/venv/` con un Python aislado del sistema.

## 2. Activar el venv

### Windows PowerShell

```powershell
.\venv\Scripts\Activate.ps1
```

> Si PowerShell bloquea la activacion, ejecuta una sola vez:
> `Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned`

### Windows CMD

```cmd
venv\Scripts\activate.bat
```

### Git Bash (Windows)

```bash
source venv/Scripts/activate
```

### macOS / Linux

```bash
source venv/bin/activate
```

Sabras que esta activo cuando el prompt muestre `(venv)` al inicio.

## 3. Instalar las dependencias

Con el venv **activado**:

```bash
pip install -r requirements.txt
```

Para verificar:

```bash
pip list
```

## 4. Ejecutar el servidor de desarrollo

Con el venv activado, desde `backend/`:

```bash
fastapi dev app/main.py
```

El servidor queda en `http://127.0.0.1:8000`.

> Tambien puedes lanzar backend + frontend en paralelo con [`../scripts/dev.sh`](../scripts/dev.sh) o [`../scripts/dev.ps1`](../scripts/dev.ps1) desde la raiz del repo.

## 5. Desactivar el venv

Cuando termines:

```bash
deactivate
```

## Notas

- La carpeta `venv/` esta ignorada por git; cada developer crea la suya localmente.
- Si agregas o actualizas dependencias, regenera `requirements.txt`:

  ```bash
  pip freeze > requirements.txt
  ```