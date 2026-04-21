#include "selec_proc.h"

#include <stdio.h>
#include <stdlib.h>

#include "bmp_utils.h"

static int flip_grey(const char *name_output, const char *input_path, int horizontal) {
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
            unsigned char gray = to_grayscale(
                src.data[src_index + 0],
                src.data[src_index + 1],
                src.data[src_index + 2]
            );

            dst.data[dst_index + 0] = gray;
            dst.data[dst_index + 1] = gray;
            dst.data[dst_index + 2] = gray;
        }
    }

    make_output_path(name_output, output_path, sizeof(output_path));
    status = bmp_write(output_path, &dst);

    bmp_free(&src);
    bmp_free(&dst);

    return status == 0 ? 0 : 3;
}

int inv_img_grey_horizontal(const char *name_output, const char *input_path) {
    return flip_grey(name_output, input_path, 1);
}

int inv_img_grey_vertical(const char *name_output, const char *input_path) {
    return flip_grey(name_output, input_path, 0);
}

int desenfoque_gris(const char *input_path, const char *name_output, int kernel_size) {
    BMPImage src;
    BMPImage dst;
    char output_path[512];
    unsigned char *gray = NULL;
    unsigned char *blurred = NULL;
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

    gray = (unsigned char *)malloc((size_t)src.width * (size_t)src.height);
    blurred = (unsigned char *)malloc((size_t)src.width * (size_t)src.height);
    if (gray == NULL || blurred == NULL) {
        free(gray);
        free(blurred);
        bmp_free(&src);
        bmp_free(&dst);
        return 4;
    }

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            int index = y * src.row_padded + x * 3;
            gray[y * src.width + x] = to_grayscale(
                src.data[index + 0],
                src.data[index + 1],
                src.data[index + 2]
            );
        }
    }

    k = kernel_size / 2;
    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            int sum = 0;
            int count = 0;

            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny < 0 || ny >= src.height) {
                    continue;
                }

                for (int dx = -k; dx <= k; dx++) {
                    int nx = x + dx;
                    if (nx < 0 || nx >= src.width) {
                        continue;
                    }

                    sum += gray[ny * src.width + nx];
                    count++;
                }
            }

            blurred[y * src.width + x] = (unsigned char)(sum / count);
        }
    }

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            int idx = y * src.row_padded + x * 3;
            unsigned char pixel = blurred[y * src.width + x];
            dst.data[idx + 0] = pixel;
            dst.data[idx + 1] = pixel;
            dst.data[idx + 2] = pixel;
        }
    }

    make_output_path(name_output, output_path, sizeof(output_path));
    status = bmp_write(output_path, &dst);

    free(gray);
    free(blurred);
    bmp_free(&src);
    bmp_free(&dst);

    return status == 0 ? 0 : 5;
}

int inv_img(const char *name_output, const char *input_path) {
    return inv_img_grey_vertical(name_output, input_path);
}
