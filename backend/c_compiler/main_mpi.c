#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "desenfoque.h"
#include "inv_img.h"

/* ── Max tasks per rank (n_images * 4 transformations) ── */
#define MAX_TASKS 4096

/* ── Per-task record ── */
typedef struct {
    int    image_index;
    char   transformation[32]; /* grey_h, color_v, color_h, blur_color */
    char   output_file[256];
    double time_seconds;
    long   pixels;
} TaskRecord;

/* ── Read BMP header: returns width*height ── */
static long bmp_pixel_count(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char xx[54];
    size_t r = fread(xx, 1, 54, f);
    fclose(f);
    if (r != 54) return 0;
    long ancho = (long)xx[20]*65536 + (long)xx[19]*256 + (long)xx[18];
    long alto  = (long)xx[24]*65536 + (long)xx[23]*256 + (long)xx[22];
    return ancho * alto;
}

int main(int argc, char *argv[]) {
    /* ── Declarations ── */
    int myrank, nprocs, provided;

    long long local_pixels  = 0;
    long long local_tasks   = 0;
    long long total_pixels  = 0;
    long long total_tasks   = 0;

    double compute_time     = 0.0;
    double global_elapsed   = 0.0;
    double total_start_time = 0.0;
    double total_end_time   = 0.0;
    double local_elapsed    = 0.0;

    long long *all_pixels   = NULL;
    long long *all_tasks    = NULL;
    double    *all_times    = NULL;

    /* Per-task tracking */
    TaskRecord *task_log    = NULL;
    int         task_log_n  = 0;

    /* ── MPI init ── */
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    /* ── Usage check ── */
    if (argc < 9) {
        if (myrank == 0)
            fprintf(stderr,
                "Uso: %s n_images grey_h color_v color_h blur_color"
                " kernel_color threads input_dir\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    /* ── Parse arguments ── */
    int   n_images     = atoi(argv[1]);
    int   grey_h       = atoi(argv[2]);
    int   color_v      = atoi(argv[3]);
    int   color_h      = atoi(argv[4]);
    int   blur_color   = atoi(argv[5]);
    int   kernel_color = atoi(argv[6]);
    int   threads      = atoi(argv[7]);
    char *input_dir    = argv[8];

    /* Allocate task log */
    task_log = calloc(MAX_TASKS, sizeof(TaskRecord));

    /* ── Rank 0 creates output directory ── */
    if (myrank == 0) {
#ifdef _WIN32
        _mkdir("../img");
#else
        mkdir("../img", 0777);
#endif
    }
    MPI_Barrier(MPI_COMM_WORLD);

    /* ── Get hostname ── */
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    /* ── Start timer ── */
    total_start_time = MPI_Wtime();
    omp_set_num_threads(threads);

    /* ── Process images with OpenMP tasks ── */
    #pragma omp parallel
    {
        #pragma omp single
        {
            int task_id = 0;

            for (int i = 1; i <= n_images; i++) {
                char input[256];
                char out_gh[256], out_cv[256], out_ch[256], out_dc[256];

                sprintf(input,  "%s/imagen_%03d.bmp",               input_dir, i);
                sprintf(out_gh, "imagen_%03d_gris_horizontal.bmp",  i);
                sprintf(out_cv, "imagen_%03d_color_vertical.bmp",   i);
                sprintf(out_ch, "imagen_%03d_color_horizontal.bmp", i);
                sprintf(out_dc, "imagen_%03d_desenfoque_color.bmp",  i);

                long pixels_per_image = bmp_pixel_count(input);

                if (grey_h) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        int idx = task_log_n++;
                        task_log[idx].image_index = i;
                        task_log[idx].pixels      = pixels_per_image;
                        strncpy(task_log[idx].transformation, "grey_h", 31);
                        strncpy(task_log[idx].output_file, out_gh, 255);
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_gh, idx)
                        {
                            /* La funcion cronometra solo el computo (sin E/S de red) */
                            inv_img_grey_horizontal(out_gh, input, &task_log[idx].time_seconds);
                        }
                    }
                    task_id++;
                }
                if (color_v) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        int idx = task_log_n++;
                        task_log[idx].image_index = i;
                        task_log[idx].pixels      = pixels_per_image;
                        strncpy(task_log[idx].transformation, "color_v", 31);
                        strncpy(task_log[idx].output_file, out_cv, 255);
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_cv, idx)
                        {
                            /* La funcion cronometra solo el computo (sin E/S de red) */
                            inv_img_color_vertical(out_cv, input, &task_log[idx].time_seconds);
                        }
                    }
                    task_id++;
                }
                if (color_h) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        int idx = task_log_n++;
                        task_log[idx].image_index = i;
                        task_log[idx].pixels      = pixels_per_image;
                        strncpy(task_log[idx].transformation, "color_h", 31);
                        strncpy(task_log[idx].output_file, out_ch, 255);
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_ch, idx)
                        {
                            /* La funcion cronometra solo el computo (sin E/S de red) */
                            inv_img_color_horizontal(out_ch, input, &task_log[idx].time_seconds);
                        }
                    }
                    task_id++;
                }
                if (blur_color) {
                    if (task_id % nprocs == myrank) {
                        local_pixels += pixels_per_image;
                        int idx = task_log_n++;
                        task_log[idx].image_index = i;
                        task_log[idx].pixels      = pixels_per_image;
                        strncpy(task_log[idx].transformation, "blur_color", 31);
                        strncpy(task_log[idx].output_file, out_dc, 255);
                        local_tasks++;
                        #pragma omp task firstprivate(input, out_dc, idx, kernel_color)
                        {
                            /* La funcion cronometra solo el computo (sin E/S de red) */
                            desenfoque_color(input, out_dc, kernel_color, &task_log[idx].time_seconds);
                        }
                    }
                    task_id++;
                }
            }
            #pragma omp taskwait

            /* Tiempo de computo del nodo = suma de los tiempos de computo
             * (sin E/S de red) de cada tarea procesada en este rank. */
            for (int t = 0; t < task_log_n; t++)
                compute_time += task_log[t].time_seconds;
        }
    }

    /* ── Wait for all ranks ── */
    MPI_Barrier(MPI_COMM_WORLD);

    total_end_time = MPI_Wtime();
    local_elapsed  = total_end_time - total_start_time;

    /* ── Reductions ── */
    MPI_Reduce(&local_elapsed, &global_elapsed, 1, MPI_DOUBLE,    MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_pixels,  &total_pixels,   1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_tasks,   &total_tasks,    1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    /* ── Gather per-rank totals ── */
    if (myrank == 0) {
        all_pixels = malloc(sizeof(long long) * nprocs);
        all_tasks  = malloc(sizeof(long long) * nprocs);
        all_times  = malloc(sizeof(double)    * nprocs);
    }
    MPI_Gather(&local_pixels, 1, MPI_LONG_LONG, all_pixels, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Gather(&local_tasks,  1, MPI_LONG_LONG, all_tasks,  1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Gather(&compute_time, 1, MPI_DOUBLE,    all_times,  1, MPI_DOUBLE,    0, MPI_COMM_WORLD);

    /* ── Rank 0: cluster summary ── */
    if (myrank == 0) {
        double pixels_per_sec = global_elapsed > 0.0
                                ? (double)total_pixels / global_elapsed
                                : 0.0;

        printf("TIME=%.6f\n",           global_elapsed);
        printf("TOTAL_PIXELS=%lld\n",   total_pixels);
        printf("PIXELS_PER_SEC=%.6e\n", pixels_per_sec);

        printf("\n");
        printf("====================================\n");
        printf(" DISTRIBUCION DE CARGA DEL CLUSTER\n");
        printf("====================================\n");
        printf("Total tareas:   %lld\n",       total_tasks);
        printf("Total pixeles:  %lld\n",       total_pixels);
        printf("Tiempo total:   %.6f s\n",     global_elapsed);
        printf("Throughput:     %.3e pix/s\n", pixels_per_sec);
        printf("====================================\n");
        printf("\nCarga por nodo:\n");

        for (int i = 0; i < nprocs; i++) {
            double pct = (total_pixels > 0)
                         ? 100.0 * all_pixels[i] / (double)total_pixels
                         : 0.0;
            printf("  Nodo %d | tareas=%lld | pixeles=%lld | carga=%.2f%% | tiempo=%.3f s\n",
                   i, all_tasks[i], all_pixels[i], pct, all_times[i]);
        }

        free(all_pixels);
        free(all_tasks);
        free(all_times);
    }

    /* ── Per-rank log with per-image detail ── */
    char logfile[64];
    sprintf(logfile, "rank_%d.log", myrank);
    FILE *log = fopen(logfile, "w");
    if (log) {
        fprintf(log, "===== MPI RANK %d | HOST: %s =====\n", myrank, hostname);
        fprintf(log, "Procesos MPI totales:  %d\n",   nprocs);
        fprintf(log, "Threads OpenMP:        %d\n",   threads);
        fprintf(log, "Tareas ejecutadas:     %lld\n", local_tasks);
        fprintf(log, "Pixeles procesados:    %lld\n", local_pixels);
        fprintf(log, "Tiempo de computo:     %.6f s (sin E/S de red)\n", compute_time);
        if (compute_time > 0)
            fprintf(log, "Pixeles/segundo:       %.3e\n",
                    (double)local_pixels / compute_time);

        fprintf(log, "\n--- Detalle por imagen ---\n");
        fprintf(log, "%-8s %-12s %-10s %-10s %s\n",
                "Imagen", "Transform.", "Pixeles", "Tiempo(s)", "Archivo salida");
        fprintf(log, "%-8s %-12s %-10s %-10s %s\n",
                "------", "----------", "-------", "---------", "--------------");

        for (int t = 0; t < task_log_n; t++) {
            fprintf(log, "%-8d %-12s %-10ld %-10.4f %s\n",
                    task_log[t].image_index,
                    task_log[t].transformation,
                    task_log[t].pixels,
                    task_log[t].time_seconds,
                    task_log[t].output_file);
        }
        fclose(log);
    }

    free(task_log);
    MPI_Finalize();
    return 0;
}