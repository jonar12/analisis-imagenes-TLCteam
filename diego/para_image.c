#include <stdio.h>
#include <stdlib.h>

#include "selec_proc.h"
#include "selec_proc_1.h"

int main(void) {
    const char *input_path = "./input/sample.bmp";
    int status = 0;

    /* Escala de grises */
    status |= inv_img_grey_horizontal("gris_horizontal", input_path);
    status |= inv_img_grey_vertical("gris_vertical", input_path);
    status |= desenfoque_gris(input_path, "gris_desenfoque_k9", 27);

    /* Color */
    status |= inv_img_color_horizontal("color_horizontal", input_path);
    status |= inv_img_color_vertical("color_vertical", input_path);
    status |= desenfoque_color(input_path, "color_desenfoque_k9", 27);

    if (status != 0) {
        printf("Se ejecutaron transformaciones, pero hubo errores en una o mas funciones.\n");
        return EXIT_FAILURE;
    }

    printf("Transformaciones completadas. Revisa la carpeta ./output\n");
    return EXIT_SUCCESS;
}
