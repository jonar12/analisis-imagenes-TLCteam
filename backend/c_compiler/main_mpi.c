#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "desenfoque.h"
#include "inv_img.h"

// Lee solo la cabecera fija de 54 bytes del BMP y devuelve ancho*alto.
// ancho esta en los bytes 18-21 (little-endian, 24 bits utiles) y alto en 22-25.
static long bmp_pixel_count(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char xx[54];
    size_t read = fread(xx, 1, 54, f);
    fclose(f);
    if (read != 54) return 0;
    long ancho = (long)xx[20]*65536 + (long)xx[19]*256 + (long)xx[18];
    long alto  = (long)xx[24]*65536 + (long)xx[23]*256 + (long)xx[22];
    return ancho * alto;
}

int main(int argc, char *argv[]) {
    int myrank, nprocs;
    int provided;

    // MPI_THREAD_FUNNELED: solo el hilo maestro de OpenMP hara llamadas MPI
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    if (argc < 9) {
        long long *all_pixels = NULL;
        long long *all_tasks = NULL;
        double *all_times = NULL;

        if (myrank == 0) {
            all_pixels = malloc(sizeof(long long) * nprocs);
            all_tasks  = malloc(sizeof(long long) * nprocs);
            all_times  = malloc(sizeof(double) * nprocs);
        }

        MPI_Gather(
            &local_pixels,
            1,
            MPI_LONG_LONG,
            all_pixels,
            1,
            MPI_LONG_LONG,
            0,
            MPI_COMM_WORLD
        );

        MPI_Gather(
            &local_tasks,
            1,
            MPI_LONG_LONG,
            all_tasks,
            1,
            MPI_LONG_LONG,
            0,
            MPI_COMM_WORLD
        );

        MPI_Gather(
            &compute_time,
            1,
            MPI_DOUBLE,
            all_times,
            1,
            MPI_DOUBLE,
            0,
            MPI_COMM_WORLD
        );
        if (myrank == 0) {
            fprintf(stderr, "Uso: %s n_images grey_h color_v color_h blur_color kernel_color threads input_dir\n", argv[0]);
            printf("\n");
            printf("====================================\n");
            printf(" DISTRIBUCION DE CARGA DEL CLUSTER\n");
            printf("====================================\n");

            printf("Total tareas: %lld\n", total_tasks);
            printf("Total pixeles: %lld\n", total_pixels);

            printf("Tiempo total job: %.6f s\n", global_elapsed);

            printf("Throughput global: %.3e pix/s\n",
                pixels_per_sec);

            printf("====================================\n");

            printf("\n");
            printf("Carga por maquina:\n");

            for (int i = 0; i < nprocs; i++) {

                double pct = 0.0;

                if (total_pixels > 0)
                    pct = 100.0 * all_pixels[i] / total_pixels;

                printf(
                    "Nodo %d | tareas=%lld | pixeles=%lld | carga=%.2f%% | tiempo=%.3f s\n",
                    i,
                    all_tasks[i],
                    all_pixels[i],
                    pct,
                    all_times[i]
                );
            }
        }
        MPI_Finalize();
        return 1;
    }

    // Todos los ranks parsean los mismos argv (mpirun los replica)
    int n_images     = atoi(argv[1]);
    int grey_h       = atoi(argv[2]);
    int color_v      = atoi(argv[3]);
    int color_h      = atoi(argv[4]);
    int blur_color   = atoi(argv[5]);
    int kernel_color = atoi(argv[6]);
    int threads      = atoi(argv[7]);
    char *input_dir  = argv[8];

    // Solo rank 0 crea el directorio de salida; los demas esperan en el barrier
    if (myrank == 0) {
        #ifdef _WIN32
            _mkdir("../img");
        #else
            mkdir("../img", 0777);
        #endif
    }
    MPI_Barrier(MPI_COMM_WORLD);

    double total_start_time = MPI_Wtime();

    omp_set_num_threads(threads);

    // Cada rank acumula los pixeles que realmente proceso en sus tareas
    long long local_pixels = 0;

    #pragma omp parallel
    {
        compute_start = MPI_Wtime();
        #pragma omp single
        {
            int task_id = 0;

            for (int i = 1; i <= n_images; i++) {
                char input[256];
                char out_gh[256], out_cv[256], out_ch[256], out_dc[256];

                sprintf(input, "%s/imagen_%03d.bmp", input_dir, i);
                sprintf(out_gh, "imagen_%03d_gris_horizontal.bmp", i);
                sprintf(out_cv, "imagen_%03d_color_vertical.bmp", i);
                sprintf(out_ch, "imagen_%03d_color_horizontal.bmp", i);
                sprintf(out_dc, "imagen_%03d_desenfoque_color.bmp", i);

                // Leemos la cabecera una sola vez por imagen
                long pixels_per_image = bmp_pixel_count(input);

                if (grey_h) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_gh)
                        inv_img_grey_horizontal(out_gh, input);
                    }
                    task_id++;
                }
                if (color_v) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_cv)
                        inv_img_color_vertical(out_cv, input);
                    }
                    task_id++;
                }
                if (color_h) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_ch)
                        inv_img_color_horizontal(out_ch, input);
                    }
                    task_id++;
                }
                if (blur_color) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_dc)
                        desenfoque_color(input, out_dc, kernel_color);
                    }
                    task_id++;
                }
            }
            #pragma omp taskwait
            compute_end = MPI_Wtime();
        }
    }
    double compute_time = compute_end - compute_start;
    // Esperamos a que TODOS los ranks terminen antes de cerrar el cronometro
    MPI_Barrier(MPI_COMM_WORLD);

    double total_end_time = MPI_Wtime();
    double local_elapsed = total_end_time - total_start_time;

    // El tiempo total del job = el del rank mas lento (MAX)
    double global_elapsed;
    MPI_Reduce(&local_elapsed, &global_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Total de pixeles procesados = suma de lo que hizo cada rank
    long long total_pixels = 0;
    long long total_tasks = 0;

    MPI_Reduce(
        &local_tasks,
        &total_tasks,
        1,
        MPI_LONG_LONG,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );
    double compute_start = 0.0;
    double compute_end = 0.0;
    MPI_Reduce(&local_pixels, &total_pixels, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (myrank == 0) {
        double pixels_per_sec = global_elapsed > 0.0 ? (double)total_pixels / global_elapsed : 0.0;
        printf("TIME=%.6f\n", global_elapsed);
        printf("TOTAL_PIXELS=%lld\n", total_pixels);
        printf("PIXELS_PER_SEC=%.6e\n", pixels_per_sec);
    }

    char logfile[64];
    sprintf(logfile, "rank_%d.log", myrank);

    FILE *log = fopen(logfile, "w");

    if (log) {

        fprintf(log, "===== MPI RANK %d =====\n", myrank);
        fprintf(log, "Procesos MPI totales: %d\n", nprocs);
        fprintf(log, "Threads OpenMP: %d\n", threads);

        fprintf(log, "Tareas ejecutadas: %lld\n", local_tasks);
        fprintf(log, "Pixeles procesados: %lld\n", local_pixels);

        fprintf(log, "Tiempo efectivo: %.6f segundos\n", compute_time);

        if (compute_time > 0)
            fprintf(log,
                    "Pixeles/segundo: %.3e\n",
                    local_pixels / compute_time);

        fclose(log);
    }

    MPI_Finalize();
    return 0;
}