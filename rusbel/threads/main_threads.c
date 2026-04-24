#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "desenfoque.h"
#include "inv_img.h"

// #define NUM_THREADS 6
// #define NUM_THREADS 12
#define NUM_THREADS 18

int main(){
    // FILE *fptr;
    double start_time, end_time;
    int status = 0;
    omp_set_num_threads(NUM_THREADS);
    const char *input_paths[3] = {
        "../input/imagen_1.bmp",
        "../input/imagen_2.bmp",
        "../input/imagen_3.bmp",
    };

    start_time = omp_get_wtime();

    #pragma omp parallel
    {
         #pragma omp single
         {
            for (int i = 0; i < 3; i++) {
                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status = 0;
                    sprintf(out_name, "img%d_gris_vertical.bmp", i + 1);
                    inv_img_grey_vertical(out_name, (char *)input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status = 0;
                    sprintf(out_name, "img%d_gris_horizontal.bmp", i + 1);
                    inv_img_grey_horizontal(out_name, (char *)input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status = 0;
                    sprintf(out_name, "img%d_desenfoque_grey", i + 1);
                    desenfoque_grey(input_paths[i], out_name, 27);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status = 0;
                    sprintf(out_name, "img%d_color_vertical.bmp", i + 1);
                    inv_img_color_vertical(out_name, (char *)input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status = 0;
                    sprintf(out_name, "img%d_color_horizontal.bmp", i + 1);
                    inv_img_color_horizontal(out_name, (char *)input_paths[i]);

                    #pragma omp critical
                    status |= local_status;
                }

                #pragma omp task firstprivate(i) shared(status, input_paths)
                {
                    char out_name[80];
                    int local_status = 0;
                    sprintf(out_name, "img%d_desenfoque_color", i + 1);
                    desenfoque_color(input_paths[i], out_name, 27);

                    #pragma omp critical
                    status |= local_status;
                }
            }

            #pragma omp taskwait
         }
    }

    end_time = omp_get_wtime();
    printf("Tiempo de ejecucion: %.6f segundos\n", end_time - start_time);

    return status;
}