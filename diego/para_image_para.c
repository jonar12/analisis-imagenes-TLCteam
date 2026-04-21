#CODIGO PRINCIPAL PARALELISADO POR DIEGO JAVIER SOLÓRZANO TRINIDAD A01808035

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include "selec_proc.h"
#include "selec_proc_1.h"

#define NUM_IMAGES 3

int main(void) {
    const char *input_paths[NUM_IMAGES] = {
        "./input/sample.bmp",
        "./input/sample2.bmp",
        "./input/sample3.bmp",
    };

    int status = 0;
    int i;
    double start_time, end_time;
    double elapsed_time;

    start_time = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (i = 0; i < NUM_IMAGES; i++) {

                /* -------- Escala de grises -------- */
                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status;
                    sprintf(out_name, "img%d_gris_horizontal", i + 1);
                    local_status = inv_img_grey_horizontal(out_name, input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status;
                    sprintf(out_name, "img%d_gris_vertical", i + 1);
                    local_status = inv_img_grey_vertical(out_name, input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status;
                    sprintf(out_name, "img%d_gris_desenfoque_k9", i + 1);
                    local_status = desenfoque_gris(input_paths[i], out_name, 27);

                    #pragma omp critical
                    status |= local_status;
                }

                /* -------- Color -------- */
                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status;
                    sprintf(out_name, "img%d_color_horizontal", i + 1);
                    local_status = inv_img_color_horizontal(out_name, input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status;
                    sprintf(out_name, "img%d_color_vertical", i + 1);
                    local_status = inv_img_color_vertical(out_name, input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status;
                    sprintf(out_name, "img%d_color_desenfoque_k9", i + 1);
                    local_status = desenfoque_color(input_paths[i], out_name, 27);

                    #pragma omp critical
                    status |= local_status;
                }
            }

            #pragma omp taskwait
        }
    }

    end_time = omp_get_wtime();
    elapsed_time = end_time - start_time;

    if (status != 0) {
        printf("Se ejecutaron transformaciones, pero hubo errores en una o mas funciones.\n");
        printf("Tiempo total paralelo: %.9f segundos\n", elapsed_time);
        return EXIT_FAILURE;
    }

    printf("Transformaciones completadas. Revisa la carpeta ./output\n");
    printf("Tiempo total paralelo: %.9f segundos\n", elapsed_time);

    return EXIT_SUCCESS;
}

//USAR 6, 12 Y 18 HILOS