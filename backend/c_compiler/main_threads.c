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

int main(int argc, char *argv[]) {

    // Leer parámetros de la línea de comandos
    int n_images = atoi(argv[1]);
    int grey_v = atoi(argv[2]);
    int grey_h = atoi(argv[3]);
    int color_v = atoi(argv[4]);
    int color_h = atoi(argv[5]);
    int blur_grey = atoi(argv[6]);
    int blur_color = atoi(argv[7]);
    int kernel_grey = atoi(argv[8]);
    int kernel_color = atoi(argv[9]);
    char *input_dir = argv[10];

    #ifdef _WIN32
        _mkdir("../img");
    #else
        mkdir("../img", 0777);
    #endif

    double total_start_time = omp_get_wtime();

    int num_threads_arr[] = {6, 12, 18};
    for (int t = 0; t < 3; t++) {
        int current_threads = num_threads_arr[t];
        omp_set_num_threads(current_threads);

        #pragma omp parallel
        {
            #pragma omp single
            {
                for (int i = 1; i <= n_images; i++) {
                    char input[256];
                    char out_gv[256], out_gh[256], out_cv[256], out_ch[256], out_dg[256], out_dc[256];

                    sprintf(input, "%s/imagen_%d.bmp", input_dir, i);
                    sprintf(out_gv, "img%d_gris_vertical.bmp", i);
                    sprintf(out_gh, "img%d_gris_horizontal.bmp", i);
                    sprintf(out_cv, "img%d_color_vertical.bmp", i);
                    sprintf(out_ch, "img%d_color_horizontal.bmp", i);
                    sprintf(out_dg, "img%d_desenfoque_grey", i);
                    sprintf(out_dc, "img%d_desenfoque_color", i);

                    if (grey_v) {
                         #pragma omp task firstprivate(input, out_gv)
                         inv_img_grey_vertical(out_gv, input);
                    }
                    if (grey_h) {
                        #pragma omp task firstprivate(input, out_gh)
                        inv_img_grey_horizontal(out_gh, input);
                    }
                    if (color_v) {
                        #pragma omp task firstprivate(input, out_cv)
                        inv_img_color_vertical(out_cv, input);
                    }
                    if (color_h) {
                        #pragma omp task firstprivate(input, out_ch)
                        inv_img_color_horizontal(out_ch, input);
                    }
                    if (blur_grey) {
                        #pragma omp task firstprivate(input, out_dg)
                        desenfoque_grey(input, out_dg, kernel_grey);
                    }
                    if (blur_color) {
                        #pragma omp task firstprivate(input, out_dc)
                        desenfoque_color(input, out_dc, kernel_color);
                    }
                }
            }
        }
    }

    double total_end_time = omp_get_wtime();
    printf("%.6f", total_end_time - total_start_time);

    return 0;
}
