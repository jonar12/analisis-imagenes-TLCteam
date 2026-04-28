from fastapi import APIRouter, UploadFile, File, HTTPException, Form # type: ignore
import json, shutil, subprocess, sys
from pathlib import Path

router = APIRouter()

C_COMPILER_DIR = Path(__file__).parent.parent.parent / "c_compiler"                                            
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

        # Delete the temporary input directory after processing
        shutil.rmtree(input_dir)

        return {
            "message": "Images were uploaded successfully",
            "output": {
                "execution_time": raw_execution_time,
                "execution_time_seconds": execution_time,
                "path": str((Path(__file__).parent.parent.parent / "img").resolve()),
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
