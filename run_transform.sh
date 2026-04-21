#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$ROOT_DIR/src"
BIN_DIR="$ROOT_DIR/bin"

order=(ihg ivg dkg ihc ivc dkc)

source_for_op() {
  case "$1" in
    ihg) echo "inversion_horizontal_gris/inversion_horizontal_gris.c" ;;
    ivg) echo "inversion_vertical_gris/inversion_vertical_gris.c" ;;
    dkg) echo "desenfoque_gris/desenfoque_gris.c" ;;
    ihc) echo "inversion_horizontal_color/inversion_horizontal_color.c" ;;
    ivc) echo "inversion_vertical_color/inversion_vertical_color.c" ;;
    dkc) echo "desenfoque_color/desenfoque_color.c" ;;
    *) return 1 ;;
  esac
}

name_for_op() {
  case "$1" in
    ihg) echo "Inversion horizontal gris" ;;
    ivg) echo "Inversion vertical gris" ;;
    dkg) echo "Desenfoque kernel gris" ;;
    ihc) echo "Inversion horizontal color" ;;
    ivc) echo "Inversion vertical color" ;;
    dkc) echo "Desenfoque kernel color" ;;
    *) return 1 ;;
  esac
}

usage() {
  cat <<'EOF'
Uso:
  ./run_transform.sh list
  ./run_transform.sh build [operacion|all]
  ./run_transform.sh run <operacion> <input.bmp> <output.bmp>
  ./run_transform.sh run-all <input.bmp> <output_dir>

Operaciones:
  ihg  inversion horizontal gris
  ivg  inversion vertical gris
  dkg  desenfoque kernel gris
  ihc  inversion horizontal color
  ivc  inversion vertical color
  dkc  desenfoque kernel color

Notas:
  - Este CLI no incluye aun el parametro de threads.
  - El comando run asume interfaz futura: <in.bmp> <out.bmp>
EOF
}

list_ops() {
  for key in "${order[@]}"; do
    printf "%-4s -> %s\n" "$key" "$(name_for_op "$key")"
  done
}

ensure_gcc() {
  if command -v gcc >/dev/null 2>&1; then
    C_COMPILER="gcc"
    return
  fi

  if command -v clang >/dev/null 2>&1; then
    C_COMPILER="clang"
    return
  fi

  echo "Error: no se encontro gcc o clang en PATH."
  exit 1
}

build_one() {
  local op="$1"
  local src_rel
  if ! src_rel="$(source_for_op "$op")"; then
    echo "Operacion invalida: $op"
    usage
    exit 1
  fi

  local src="$SRC_DIR/$src_rel"
  local out="$BIN_DIR/$op"

  mkdir -p "$BIN_DIR"
  ensure_gcc

  if [[ ! -f "$src" ]]; then
    echo "No existe el archivo fuente: $src"
    exit 1
  fi

  echo "Compilando $op -> $out"
  "$C_COMPILER" -O2 -std=c11 -Wall -Wextra "$src" -o "$out"
}

run_one() {
  local op="$1"
  local input="$2"
  local output="$3"
  local bin="$BIN_DIR/$op"

  if ! source_for_op "$op" >/dev/null; then
    echo "Operacion invalida: $op"
    usage
    exit 1
  fi

  if [[ ! -f "$input" ]]; then
    echo "No existe input: $input"
    exit 1
  fi

  if [[ ! -x "$bin" ]]; then
    echo "No existe binario para '$op'. Intentando compilar..."
    build_one "$op"
  fi

  echo "Ejecutando $op"
  "$bin" "$input" "$output"
  echo "Salida: $output"
}

run_all() {
  local input="$1"
  local out_dir="$2"

  mkdir -p "$out_dir"
  for op in "${order[@]}"; do
    local out_file="$out_dir/${op}_$(basename "$input")"
    run_one "$op" "$input" "$out_file"
  done
}

main() {
  local cmd="${1:-}"

  case "$cmd" in
    list)
      list_ops
      ;;
    build)
      local op="${2:-all}"
      if [[ "$op" == "all" ]]; then
        for k in "${order[@]}"; do
          build_one "$k"
        done
      else
        build_one "$op"
      fi
      ;;
    run)
      if [[ $# -ne 4 ]]; then
        usage
        exit 1
      fi
      run_one "$2" "$3" "$4"
      ;;
    run-all)
      if [[ $# -ne 3 ]]; then
        usage
        exit 1
      fi
      run_all "$2" "$3"
      ;;
    -h|--help|help|"")
      usage
      ;;
    *)
      echo "Comando invalido: $cmd"
      usage
      exit 1
      ;;
  esac
}

main "$@"