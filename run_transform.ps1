#!/usr/bin/env pwsh
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RootDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SrcDir = Join-Path $RootDir 'src'
$BinDir = Join-Path $RootDir 'bin'
$Order = @('ihg','ivg','dkg','ihc','ivc','dkc')

function Get-SourceForOp {
    param([Parameter(Mandatory = $true)][string]$Op)

    switch ($Op) {
        'ihg' { 'inversion_horizontal_gris/inversion_horizontal_gris.c' }
        'ivg' { 'inversion_vertical_gris/inversion_vertical_gris.c' }
        'dkg' { 'desenfoque_gris/desenfoque_gris.c' }
        'ihc' { 'inversion_horizontal_color/inversion_horizontal_color.c' }
        'ivc' { 'inversion_vertical_color/inversion_vertical_color.c' }
        'dkc' { 'desenfoque_color/desenfoque_color.c' }
        default { throw "Operacion invalida: $Op" }
    }
}

function Get-NameForOp {
    param([Parameter(Mandatory = $true)][string]$Op)

    switch ($Op) {
        'ihg' { 'Inversion horizontal gris' }
        'ivg' { 'Inversion vertical gris' }
        'dkg' { 'Desenfoque kernel gris' }
        'ihc' { 'Inversion horizontal color' }
        'ivc' { 'Inversion vertical color' }
        'dkc' { 'Desenfoque kernel color' }
        default { throw "Operacion invalida: $Op" }
    }
}

function Show-Usage {
    @'
Uso:
  ./run_transform.ps1 list
  ./run_transform.ps1 build [operacion|all]
  ./run_transform.ps1 run <operacion> <input.bmp> <output.bmp>
  ./run_transform.ps1 run-all <input.bmp> <output_dir>

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
'@ | Write-Host
}

function Get-CCompiler {
    $gcc = Get-Command gcc -ErrorAction SilentlyContinue
    if ($null -ne $gcc) {
        return 'gcc'
    }

    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -ne $clang) {
        return 'clang'
    }

    throw 'Error: no se encontro gcc o clang en PATH.'
}

function Build-One {
    param([Parameter(Mandatory = $true)][string]$Op)

    $srcRel = Get-SourceForOp -Op $Op
    $src = Join-Path $SrcDir $srcRel
    $out = Join-Path $BinDir $Op

    if (-not (Test-Path -LiteralPath $BinDir)) {
        New-Item -ItemType Directory -Path $BinDir | Out-Null
    }

    if (-not (Test-Path -LiteralPath $src)) {
        throw "No existe el archivo fuente: $src"
    }

    $compiler = Get-CCompiler
    Write-Host "Compilando $Op -> $out"
    & $compiler '-O2' '-std=c11' '-Wall' '-Wextra' $src '-o' $out
    if ($LASTEXITCODE -ne 0) {
        throw "Error al compilar $Op"
    }
}

function Run-One {
    param(
        [Parameter(Mandatory = $true)][string]$Op,
        [Parameter(Mandatory = $true)][string]$Input,
        [Parameter(Mandatory = $true)][string]$Output
    )

    [void](Get-SourceForOp -Op $Op)

    if (-not (Test-Path -LiteralPath $Input)) {
        throw "No existe input: $Input"
    }

    $bin = Join-Path $BinDir $Op
    if (-not (Test-Path -LiteralPath $bin)) {
        Write-Host "No existe binario para '$Op'. Intentando compilar..."
        Build-One -Op $Op
    }

    Write-Host "Ejecutando $Op"
    & $bin $Input $Output
    if ($LASTEXITCODE -ne 0) {
        throw "Error al ejecutar $Op"
    }

    Write-Host "Salida: $Output"
}

function Run-All {
    param(
        [Parameter(Mandatory = $true)][string]$Input,
        [Parameter(Mandatory = $true)][string]$OutDir
    )

    if (-not (Test-Path -LiteralPath $OutDir)) {
        New-Item -ItemType Directory -Path $OutDir | Out-Null
    }

    foreach ($op in $Order) {
        $baseName = [System.IO.Path]::GetFileName($Input)
        $outFile = Join-Path $OutDir ("{0}_{1}" -f $op, $baseName)
        Run-One -Op $op -Input $Input -Output $outFile
    }
}

function List-Ops {
    foreach ($op in $Order) {
        $name = Get-NameForOp -Op $op
        Write-Host ("{0,-4} -> {1}" -f $op, $name)
    }
}

try {
    if ($args.Count -eq 0) {
        Show-Usage
        exit 0
    }

    $cmd = $args[0]
    switch ($cmd) {
        'list' {
            List-Ops
        }
        'build' {
            $op = if ($args.Count -ge 2) { $args[1] } else { 'all' }
            if ($op -eq 'all') {
                foreach ($k in $Order) {
                    Build-One -Op $k
                }
            } else {
                Build-One -Op $op
            }
        }
        'run' {
            if ($args.Count -ne 4) {
                Show-Usage
                throw 'Parametros invalidos para run.'
            }

            Run-One -Op $args[1] -Input $args[2] -Output $args[3]
        }
        'run-all' {
            if ($args.Count -ne 3) {
                Show-Usage
                throw 'Parametros invalidos para run-all.'
            }

            Run-All -Input $args[1] -OutDir $args[2]
        }
        'help' {
            Show-Usage
        }
        '-h' {
            Show-Usage
        }
        '--help' {
            Show-Usage
        }
        default {
            Show-Usage
            throw "Comando invalido: $cmd"
        }
    }
}
catch {
    Write-Error $_
    exit 1
}
