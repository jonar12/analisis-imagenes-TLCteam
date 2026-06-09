from pathlib import Path

from fastapi import FastAPI # type: ignore
from fastapi.middleware.cors import CORSMiddleware # type: ignore
from fastapi.staticfiles import StaticFiles # type: ignore
from app.api import router

app = FastAPI(title="Sentiment Analysis API")
IMG_DIR = Path(__file__).parent.parent / "img"
IMG_DIR.mkdir(parents=True, exist_ok=True)

# CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://127.0.0.1:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Routers
app.include_router(router.router, prefix="/img")
app.mount("/processed", StaticFiles(directory=IMG_DIR), name="processed")

@app.get("/")
def read_root():
    return {"Hello": "World"}
