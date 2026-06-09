from fastapi import APIRouter # type: ignore
from app.api import compiler

router = APIRouter()

router.include_router(compiler.router)