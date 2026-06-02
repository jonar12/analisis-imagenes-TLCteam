from fastapi import APIRouter, UploadFile, File, HTTPException, Form, Body # type: ignore
from fastapi.responses import StreamingResponse # type: ignore
import io, json, os, re, shutil, subprocess, sys, zipfile
from pathlib import Path

router = APIRouter()

C_COMPILER_DIR = Path(__file__).parent.parent.parent / "c_compiler"                                            
IMG_DIR = Path(__file__).parent.parent.parent / "img"
BINARY_NAME = "main_mpi.exe" if sys.platform == "win32" else "main_mpi"                              
ALLOWED_THREADS = {6, 12, 18}
MAX_IMAGES = 250
KERNEL_MIN = 55
KERNEL_MAX = 155

# Configuracion del cluster MPI

# HOSTFILE: archivo con la lista de nodos del cluster.
HOSTFILE = Path("/home/mpiuser/cloud/my_host")
# MPI_PROCESSES: fallback local cuando no hay cluster. Default 1 para no saturar la maquina del master y OpenMP paraleliza dentro del unico rank.
MPI_PROCESSES = "1"

TRANSFORMATION_LABELS = {
    "grey_h": "Escala de grises horizontal",
    "color_v": "Color vertical",
    "color_h": "Color horizontal",
    "blur_color": "Desenfoque a color",
}
OUTPUT_PATTERNS = {
    "grey_h": "img{index:03d}_gris_horizontal.bmp",
    "color_v": "img{index:03d}_color_vertical.bmp",
    "color_h": "img{index:03d}_color_horizontal.bmp",
    "blur_color": "img{index:03d}_desenfoque_color.bmp",
}

def expected_output_images(n_images: int, opts: dict):
    images = []
    for index in range(1, n_images + 1):
        for flag, filename_pattern in OUTPUT_PATTERNS.items():
            if opts.get(flag, False):
                filename = filename_pattern.format(index=index)
                images.append({
                    "filename": filename,
                    "image_index": index,
                    "transformation": flag,
                    "label": TRANSFORMATION_LABELS[flag],
                })
    return images

@router.post("/download-results")
async def download_results(filenames: list[str] = Body(...)):
    try:
        if not filenames:
            raise HTTPException(status_code=400, detail="No hay archivos para descargar")

        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, "w", compression=zipfile.ZIP_DEFLATED) as zip_file:
            for filename in filenames:
                safe_name = Path(filename).name
                if safe_name != filename or not safe_name.lower().endswith(".bmp"):
                    raise HTTPException(status_code=400, detail=f"Archivo invalido: {filename}")

                file_path = IMG_DIR / safe_name
                if not file_path.exists():
                    raise HTTPException(status_code=404, detail=f"No existe: {safe_name}")

                zip_file.write(file_path, arcname=safe_name)

        buffer.seek(0)
        headers = {"Content-Disposition": 'attachment; filename="resultados_tlc.zip"'}
        return StreamingResponse(buffer, media_type="application/zip", headers=headers)

    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail={
                "error": "Error creating download",
                "message": str(e),
            },
        )

@router.post("/compiler")
async def img_processor(images: list[UploadFile] = File(...), options: str = Form(...)): # JSON como string
    input_dir = None
    try:
        # Parse options
        opts = json.loads(options)
        # print("Received options:", opts)  # Debug: print parsed options

        if len(images) > MAX_IMAGES:
            raise HTTPException(status_code=400, detail=f"Solo se permiten hasta {MAX_IMAGES} imagenes BMP")

        invalid_images = [
            img.filename for img in images
            if not img.filename or not img.filename.lower().endswith(".bmp")
        ]
        if invalid_images:
            raise HTTPException(
                status_code=400,
                detail=f"Todos los archivos deben ser .bmp. Invalidos: {', '.join(invalid_images)}",
            )

        threads = int(opts.get("threads", 6))
        if threads not in ALLOWED_THREADS:
            raise HTTPException(status_code=400, detail="threads debe ser 6, 12 o 18")

        kernel_color = int(opts.get("kernel_color", KERNEL_MIN))
        if opts.get("blur_color", False) and (
            kernel_color < KERNEL_MIN or kernel_color > KERNEL_MAX or kernel_color % 2 == 0
        ):
            raise HTTPException(
                status_code=400,
                detail=f"kernel_color debe ser impar y estar entre {KERNEL_MIN} y {KERNEL_MAX}",
            )

        # Save uploaded images to a temporary directory
        input_dir = Path(__file__).parent.parent.parent / "input"
        if input_dir.exists():
            shutil.rmtree(input_dir)
        input_dir.mkdir(parents=True, exist_ok=True)
        for i, img in enumerate(images, start=1):
            dest = input_dir / f"imagen_{i:03d}.bmp"
            with open(dest, "wb") as buffer:
                shutil.copyfileobj(img.file, buffer)
        
        # Define the complete path for the compiled binary
        BINARY_PATH = C_COMPILER_DIR / BINARY_NAME

        # Compilar el binario MPI. mpicc envuelve al compilador del sistema
        if sys.platform == "darwin":
            # En macOS el OpenMP de Homebrew necesita estos flags explicitos.
            compile_cmd = [
                "mpicc", "-Xclang", "-fopenmp",
                "-L/opt/homebrew/opt/libomp/lib",
                "-I/opt/homebrew/opt/libomp/include",
                "-lomp", "main_mpi.c", "-o", str(BINARY_PATH),
            ]
        else:
            compile_cmd = ["mpicc", "-fopenmp", "main_mpi.c", "-o", str(BINARY_PATH)]

        compile_result = subprocess.run(
            compile_cmd, cwd=C_COMPILER_DIR, capture_output=True, text=True,
        )
        if compile_result.returncode != 0:
            raise HTTPException(
                status_code=500,
                detail={
                    "error": "Error compilando el binario MPI",
                    "message": compile_result.stderr.strip(),
                },
            )

        # Define the flags in the same order as expected by the C program
        flags = ["grey_h", "color_v", "color_h", "blur_color"]
        selected_transformations = [
            TRANSFORMATION_LABELS[flag] for flag in flags if opts.get(flag, False)
        ]
        if not selected_transformations:
            raise HTTPException(status_code=400, detail="Selecciona al menos una transformacion")

        expected_images = expected_output_images(len(images), opts)
        IMG_DIR.mkdir(parents=True, exist_ok=True)
        for output_image in expected_images:
            output_path = IMG_DIR / output_image["filename"]
            if output_path.exists():
                output_path.unlink()

        # Prepare arguments matching C: n_images, flags, kernels, threads, input_dir
        args = [
            str(len(images)),
            *(str(int(opts.get(f, False))) for f in flags),
            str(kernel_color),
            str(threads),
            str(input_dir.resolve()),
        ]
        # print("Running binary with args:", args)  # Debug: print arguments for the binary

        # Construcción del comando mpirun.
        mpi_cmd = ["mpirun"]
        if HOSTFILE.exists():
            # Con hostfile: 1 proceso por maquina (-npernode 1). mpirun cuenta los nodos del archivo host
            mpi_cmd += ["--hostfile", str(HOSTFILE), "-npernode", "1"]
        else:
            # Sin hostfile: corre local con MPI_PROCESSES procesos (util para pruebas locales).
            mpi_cmd += ["-np", str(MPI_PROCESSES)]
        mpi_cmd += [str(BINARY_PATH), *args]

        run_result = subprocess.run(mpi_cmd, capture_output=True, text=True, cwd=C_COMPILER_DIR)
        if run_result.returncode != 0:
            raise HTTPException(
                status_code=500,
                detail={
                    "error": "Error ejecutando el binario MPI",
                    "message": run_result.stderr.strip() or run_result.stdout.strip(),
                },
            )

        raw_stdout = run_result.stdout.strip()

        log_files = []

        for log_file in C_COMPILER_DIR.glob("rank_*.log"):
            log_files.append({
                "name": log_file.name,
                "content": log_file.read_text()
            })

        # Parsea pares KEY=valor del stdout; valor acepta int, float y notacion cientifica (1.234e+08).
        def _extract(key: str, cast=float, default=0.0):
            match = re.search(rf"{key}=([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)", raw_stdout)
            if not match:
                return default
            try:
                return cast(match.group(1))
            except ValueError:
                return default

        execution_time  = _extract("TIME", float)
        total_pixels    = _extract("TOTAL_PIXELS", lambda v: int(float(v)))
        pixels_per_sec  = _extract("PIXELS_PER_SEC", float)

        output_images = []
        for output_image in expected_images:
            output_path = IMG_DIR / output_image["filename"]
            if output_path.exists():
                output_images.append({
                    **output_image,
                    "url": f"/processed/{output_image['filename']}",
                    "size": output_path.stat().st_size,
                })

        return {
            "message": "Images were uploaded successfully",
            "output": {
                "execution_time": f"{execution_time:.6f}",
                "execution_time_seconds": execution_time,
                "path": str(IMG_DIR.resolve()),
                "images": output_images,
            },
            "metrics": {
                "images": len(images),
                "threads": threads,
                "transformations": selected_transformations,
                "kernel_color": kernel_color,
                "execution_time_seconds": execution_time,
                "total_pixels": total_pixels,
                "pixels_per_second": f"{pixels_per_sec:.3e}",
            },
            "cluster_metrics": {
                "logs": log_files
            },
        }
                
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail={
                "error": "Error processing images",
                "message": str(e)
            }
        )
    finally:
        # Limpiar el directorio temporal de entrada pase lo que pase.
        if input_dir is not None and input_dir.exists():
            shutil.rmtree(input_dir, ignore_errors=True)
