#ifndef SELEC_PROC_1_H
#define SELEC_PROC_1_H

int inv_img_color_horizontal(const char *name_output, const char *input_path);
int inv_img_color_vertical(const char *name_output, const char *input_path);
int desenfoque_color(const char *input_path, const char *name_output, int kernel_size);

/* Alias compatible*/
int inv_img_color(const char *name_output, const char *input_path);
int desenfoque(const char *input_path, const char *name_output, int kernel_size);

#endif
