from fastapi import APIRouter, UploadFile, File, HTTPException, Form # type: ignore
import json, os, shutil, subprocess, sys
from pathlib import Path

router = APIRouter()

C_COMPILER_DIR = Path(__file__).parent.parent.parent / "c_compiler"                                            
BINARY_NAME = "main_threads.exe" if sys.platform == "win32" else "main_threads"                              

@router.post("/compiler")
async def img_processor(images: list[UploadFile] = File(...), options: str = Form(...)): # JSON como string
    try:
        # Parse options
        opts = json.loads(options)

        # Save uploaded images to a temporary directory
        input_dir = "./input"
        os.makedirs(input_dir, exist_ok=True)
        for i, img in enumerate(images, start=1):
            dest = f"{input_dir}/imagen_{i}.bmp"
            with open(dest, "wb") as buffer:
                shutil.copyfileobj(img.file, buffer)
        
        # Define the complete path for the compiled binary
        BINARY_PATH = C_COMPILER_DIR / BINARY_NAME

        # Compile the C code
        subprocess.run([
            "gcc", "-fopenmp", "main_threads.c", "-o", str(BINARY_PATH)
        ], cwd=C_COMPILER_DIR, check=True)

        # Define the flags in the same order as expected by the C program
        flags = ["grey_v", "grey_h", "color_v", "color_h", "blur_grey", "blur_color"]

        # Prepare arguments
        args = [
            str(len(images)),
            *(str(int(opts.get(f, False))) for f in flags),
            str(opts.get("kernel_grey", 0)),
            str(opts.get("kernel_color", 0)),
            str(input_dir.resolve()),
        ]

        # Run the compiled binary
        run_result = subprocess.run([str(BINARY_PATH), *args], capture_output=True, text=True, check=True)

        return {
            "message": "Images were uploaded successfully",
            "output": {
                "execution_time": run_result.stdout,  # You can modify the C code to print execution time and capture it here
            }
        }
                
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail={
                "error": "Error processing images",
                "message": str(e)
            }
        )