# Análisis de Imágenes con OpenMP

## Contribuidores
- Jonathan Armando Arredondo Hernandez | A01737788
- Fernando Maggi Llerandi | A01736935
- Diego Javier Solorzano Trinidad | A01808035
- Rusbel Alejandro Morales Mendez | A01737814
- Pablo Andre Coca Murillo | A01737438

## Descripción
Este proyecto realiza procesamiento de imágenes en formato BMP utilizando paralelización con OpenMP en C. El objetivo principal es aplicar múltiples filtros (desenfoque e inversión) a un conjunto de imágenes de manera concurrente, optimizando y reduciendo significativamente el tiempo de ejecución.

## Implementación
El programa principal (`main_threads.c`) está diseñado para procesar 3 imágenes de entrada (`imagen_1.bmp`, `imagen_2.bmp`, `imagen_3.bmp`). Para cada imagen, se aplican 6 transformaciones independientes, resultando en un total de 18 tareas de procesamiento:
- Inversión vertical (escala de grises y color)
- Inversión horizontal (escala de grises y color)
- Desenfoque o *blur* (escala de grises y color)

El procesamiento de las imágenes se delega a funciones definidas en módulos separados:
- `desenfoque.h`: Contiene la lógica para aplicar el filtro de desenfoque a las imágenes.
- `inv_img.h`: Contiene la lógica para invertir o rotar las imágenes de forma vertical y horizontal.

El programa crea automáticamente el directorio de salida `img/` (si no existe) y guarda allí todas las imágenes resultantes.

## Paralelización con OpenMP
La concurrencia y paralelización se implementan utilizando la API de OpenMP mediante las directivas `#pragma omp parallel` y `#pragma omp sections`.

- **Asignación de Tareas**: Cada una de las 18 operaciones de procesamiento se define dentro de un bloque individual `#pragma omp section`. Esto permite que OpenMP distribuya dinámicamente las tareas entre los hilos disponibles.
- **Pruebas de Rendimiento**: El programa evalúa automáticamente el rendimiento ejecutando el lote completo de tareas bajo diferentes configuraciones de hilos (`6`, `12` y `18` hilos) utilizando `omp_set_num_threads()`.
- **Medición de Tiempos**: Se calcula el tiempo de ejecución exacto para cada configuración utilizando `omp_get_wtime()`. Al finalizar, se imprime en consola el tiempo que tomó procesar todas las imágenes con la cantidad de hilos específica.

## Requisitos y Compilación
- Compilador de C compatible con OpenMP (como `gcc` o `clang` con soporte OpenMP).
- Imágenes de entrada en formato BMP (`imagen_1.bmp`, `imagen_2.bmp`, `imagen_3.bmp`) ubicadas en una carpeta `input/` en el directorio padre del ejecutable.

Para compilar el código, asegúrese de incluir la bandera de OpenMP:

- En Windows
```bash
gcc -fopenmp threads/main_threads.c -o main_threads
```

- En SOs UNIX (Linux o Mac)
```bash
clang -Xclang -fopenmp -L/opt/homebrew/opt/libomp/lib -I/opt/homebrew/opt/libomp/include -lomp main_threads.c -o main_threads
```

Para ejecutar el programa:
```bash
./main_threads
```
