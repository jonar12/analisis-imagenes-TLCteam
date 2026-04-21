#ifndef SELEC_PROC_H
#define SELEC_PROC_H

int inv_img_grey_horizontal(const char *name_output, const char *input_path);
int inv_img_grey_vertical(const char *name_output, const char *input_path);
int desenfoque_gris(const char *input_path, const char *name_output, int kernel_size);

/* Alias compatible */
int inv_img(const char *name_output, const char *input_path);

#endif
