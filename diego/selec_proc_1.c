#include "selec_proc_1.h"

#include <stdlib.h>

#include "bmp_utils.h"

static int flip_color(const char *name_output, const char *input_path, int horizontal) {
    BMPImage src;
    BMPImage dst;
    char output_path[512];
    int status;

    status = bmp_read(input_path, &src);
    if (status != 0) {
        return 1;
    }

    status = bmp_clone(&src, &dst);
    if (status != 0) {
        bmp_free(&src);
        return 2;
    }

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            int src_index = y * src.row_padded + x * 3;
            int out_x = horizontal ? (src.width - 1 - x) : x;
            int out_y = horizontal ? y : (src.height - 1 - y);
            int dst_index = out_y * src.row_padded + out_x * 3;

            dst.data[dst_index + 0] = src.data[src_index + 0];
            dst.data[dst_index + 1] = src.data[src_index + 1];
            dst.data[dst_index + 2] = src.data[src_index + 2];
        }
    }

    make_output_path(name_output, output_path, sizeof(output_path));
    status = bmp_write(output_path, &dst);

    bmp_free(&src);
    bmp_free(&dst);

    return status == 0 ? 0 : 3;
}

int inv_img_color_horizontal(const char *name_output, const char *input_path) {
    return flip_color(name_output, input_path, 1);
}

int inv_img_color_vertical(const char *name_output, const char *input_path) {
    return flip_color(name_output, input_path, 0);
}

int desenfoque_color(const char *input_path, const char *name_output, int kernel_size) {
    BMPImage src;
    BMPImage dst;
    char output_path[512];
    int k;
    int status;

    if (kernel_size <= 0 || kernel_size % 2 == 0) {
        return 1;
    }

    status = bmp_read(input_path, &src);
    if (status != 0) {
        return 2;
    }

    status = bmp_clone(&src, &dst);
    if (status != 0) {
        bmp_free(&src);
        return 3;
    }

    k = kernel_size / 2;
    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            int sum_b = 0;
            int sum_g = 0;
            int sum_r = 0;
            int count = 0;

            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny < 0 || ny >= src.height) {
                    continue;
                }

                for (int dx = -k; dx <= k; dx++) {
                    int nx = x + dx;
                    int idx;

                    if (nx < 0 || nx >= src.width) {
                        continue;
                    }

                    idx = ny * src.row_padded + nx * 3;
                    sum_b += src.data[idx + 0];
                    sum_g += src.data[idx + 1];
                    sum_r += src.data[idx + 2];
                    count++;
                }
            }

            {
                int out_idx = y * src.row_padded + x * 3;
                dst.data[out_idx + 0] = (unsigned char)(sum_b / count);
                dst.data[out_idx + 1] = (unsigned char)(sum_g / count);
                dst.data[out_idx + 2] = (unsigned char)(sum_r / count);
            }
        }
    }

    make_output_path(name_output, output_path, sizeof(output_path));
    status = bmp_write(output_path, &dst);

    bmp_free(&src);
    bmp_free(&dst);

    return status == 0 ? 0 : 4;
}

int inv_img_color(const char *name_output, const char *input_path) {
    return inv_img_color_vertical(name_output, input_path);
}

int desenfoque(const char *input_path, const char *name_output, int kernel_size) {
    return desenfoque_color(input_path, name_output, kernel_size);
}
