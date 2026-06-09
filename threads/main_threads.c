#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "desenfoque.h"
#include "inv_img.h"

int main(){
#ifdef _WIN32
    _mkdir("../img");
#else
    mkdir("../img", 0777);
#endif

    int num_threads_arr[] = {6, 12, 18};
    for (int i = 0; i < 3; i++) {
        int current_threads = num_threads_arr[i];
        double start_time, end_time;
        omp_set_num_threads(current_threads);

        start_time = omp_get_wtime();

        #pragma omp parallel
    {
        #pragma omp sections
        {
            // Imagen 1
            #pragma omp section
            inv_img_grey_vertical("img1_gris_vertical.bmp", "../input/imagen_1.bmp");
            #pragma omp section
            inv_img_grey_horizontal("img1_gris_horizontal.bmp", "../input/imagen_1.bmp");
            #pragma omp section
            desenfoque_grey("../input/imagen_1.bmp", "img1_desenfoque_grey", 27);
            #pragma omp section
            inv_img_color_vertical("img1_color_vertical.bmp", "../input/imagen_1.bmp");
            #pragma omp section
            inv_img_color_horizontal("img1_color_horizontal.bmp", "../input/imagen_1.bmp");
            #pragma omp section
            desenfoque_color("../input/imagen_1.bmp", "img1_desenfoque_color", 27);

            // Imagen 2
            #pragma omp section
            inv_img_grey_vertical("img2_gris_vertical.bmp", "../input/imagen_2.bmp");
            #pragma omp section
            inv_img_grey_horizontal("img2_gris_horizontal.bmp", "../input/imagen_2.bmp");
            #pragma omp section
            desenfoque_grey("../input/imagen_2.bmp", "img2_desenfoque_grey", 27);
            #pragma omp section
            inv_img_color_vertical("img2_color_vertical.bmp", "../input/imagen_2.bmp");
            #pragma omp section
            inv_img_color_horizontal("img2_color_horizontal.bmp", "../input/imagen_2.bmp");
            #pragma omp section
            desenfoque_color("../input/imagen_2.bmp", "img2_desenfoque_color", 27);

            // Imagen 3
            #pragma omp section
            inv_img_grey_vertical("img3_gris_vertical.bmp", "../input/imagen_3.bmp");
            #pragma omp section
            inv_img_grey_horizontal("img3_gris_horizontal.bmp", "../input/imagen_3.bmp");
            #pragma omp section
            desenfoque_grey("../input/imagen_3.bmp", "img3_desenfoque_grey", 27);
            #pragma omp section
            inv_img_color_vertical("img3_color_vertical.bmp", "../input/imagen_3.bmp");
            #pragma omp section
            inv_img_color_horizontal("img3_color_horizontal.bmp", "../input/imagen_3.bmp");
            #pragma omp section
            desenfoque_color("../input/imagen_3.bmp", "img3_desenfoque_color", 27);
        }
    }

        end_time = omp_get_wtime();
        printf("Threads utilizados: %d\n", current_threads);
        printf("Tiempo de ejecucion: %.6f segundos\n", end_time - start_time);
    }

    return 0;
}
