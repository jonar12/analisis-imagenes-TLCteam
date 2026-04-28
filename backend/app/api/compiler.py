from fastapi import APIRouter, UploadFile, File, HTTPException, Form, Body # type: ignore
from fastapi.responses import StreamingResponse # type: ignore
import io, json, shutil, subprocess, sys, zipfile
from pathlib import Path

router = APIRouter()

C_COMPILER_DIR = Path(__file__).parent.parent.parent / "c_compiler"                                            
IMG_DIR = Path(__file__).parent.parent.parent / "img"
BINARY_NAME = "main_threads.exe" if sys.platform == "win32" else "main_threads"                              
ALLOWED_THREADS = {6, 12, 18}
TRANSFORMATION_LABELS = {
    "grey_v": "Escala de grises vertical",
    "grey_h": "Escala de grises horizontal",
    "color_v": "Color vertical",
    "color_h": "Color horizontal",
    "blur_grey": "Blur en escala de grises",
    "blur_color": "Blur en color",
}
OUTPUT_PATTERNS = {
    "grey_v": "img{index}_gris_vertical.bmp",
    "grey_h": "img{index}_gris_horizontal.bmp",
    "color_v": "img{index}_color_vertical.bmp",
    "color_h": "img{index}_color_horizontal.bmp",
    "blur_grey": "img{index}_desenfoque_grey.bmp",
    "blur_color": "img{index}_desenfoque_color.bmp",
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
    try:
        # Parse options
        opts = json.loads(options)
        print("Received options:", opts)  # Debug: print parsed options

        if len(images) > 10:
            raise HTTPException(status_code=400, detail="Solo se permiten hasta 10 imagenes BMP")

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

        # Save uploaded images to a temporary directory
        input_dir = Path(__file__).parent.parent.parent / "input"
        if input_dir.exists():
            shutil.rmtree(input_dir)
        input_dir.mkdir(parents=True, exist_ok=True)
        for i, img in enumerate(images, start=1):
            dest = input_dir / f"imagen_{i}.bmp"
            with open(dest, "wb") as buffer:
                shutil.copyfileobj(img.file, buffer)
        
        # Define the complete path for the compiled binary
        BINARY_PATH = C_COMPILER_DIR / BINARY_NAME

        # Compile the C code
        if sys.platform == "darwin":
            compile_cmd = [
                "clang", "-Xclang", "-fopenmp",
                "-L/opt/homebrew/opt/libomp/lib",
                "-I/opt/homebrew/opt/libomp/include",
                "-lomp", "main_threads.c", "-o", str(BINARY_PATH)
            ]
        else:
            compile_cmd = ["gcc", "-fopenmp", "main_threads.c", "-o", str(BINARY_PATH)]
        subprocess.run(compile_cmd, cwd=C_COMPILER_DIR, check=True)

        # Define the flags in the same order as expected by the C program
        flags = ["grey_v", "grey_h", "color_v", "color_h", "blur_grey", "blur_color"]
        selected_transformations = [
            TRANSFORMATION_LABELS[flag] for flag in flags if opts.get(flag, False)
        ]
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
            str(opts.get("kernel_grey", 0)),
            str(opts.get("kernel_color", 0)),
            str(threads),
            str(input_dir.resolve()),
        ]
        print("Running binary with args:", args)  # Debug: print arguments for the binary

        # Run the compiled binary
        run_result = subprocess.run([str(BINARY_PATH), *args], capture_output=True, text=True, check=True, cwd=C_COMPILER_DIR)
        raw_execution_time = run_result.stdout.strip()
        execution_time = float(raw_execution_time.splitlines()[-1]) if raw_execution_time else 0
        output_images = []
        for output_image in expected_images:
            output_path = IMG_DIR / output_image["filename"]
            if output_path.exists():
                output_images.append({
                    **output_image,
                    "url": f"/processed/{output_image['filename']}",
                    "size": output_path.stat().st_size,
                })

        # Delete the temporary input directory after processing
        shutil.rmtree(input_dir)

        return {
            "message": "Images were uploaded successfully",
            "output": {
                "execution_time": raw_execution_time,
                "execution_time_seconds": execution_time,
                "path": str(IMG_DIR.resolve()),
                "images": output_images,
            },
            "metrics": {
                "images": len(images),
                "threads": threads,
                "transformations": selected_transformations,
                "kernel_grey": int(opts.get("kernel_grey", 0)),
                "kernel_color": int(opts.get("kernel_color", 0)),
                "execution_time_seconds": execution_time,
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
