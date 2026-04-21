#include "bmp_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int has_bmp_extension(const char *name) {
    size_t len = strlen(name);
    if (len < 4) {
        return 0;
    }

    return (name[len - 4] == '.' || name[len - 4] == '.') &&
           (name[len - 3] == 'b' || name[len - 3] == 'B') &&
           (name[len - 2] == 'm' || name[len - 2] == 'M') &&
           (name[len - 1] == 'p' || name[len - 1] == 'P');
}

int make_output_path(const char *name_output, char *out_path, size_t out_size) {
    if (name_output == NULL || out_path == NULL || out_size == 0) {
        return 1;
    }

    if (strchr(name_output, '/') != NULL || strchr(name_output, '\\') != NULL) {
        if (has_bmp_extension(name_output)) {
            snprintf(out_path, out_size, "%s", name_output);
        } else {
            snprintf(out_path, out_size, "%s.bmp", name_output);
        }
        return 0;
    }

    mkdir("./output", 0777);

    if (has_bmp_extension(name_output)) {
        snprintf(out_path, out_size, "./output/%s", name_output);
    } else {
        snprintf(out_path, out_size, "./output/%s.bmp", name_output);
    }

    return 0;
}

unsigned char to_grayscale(unsigned char b, unsigned char g, unsigned char r) {
    float value = 0.114f * (float)b + 0.587f * (float)g + 0.299f * (float)r;
    if (value < 0.0f) {
        value = 0.0f;
    }
    if (value > 255.0f) {
        value = 255.0f;
    }
    return (unsigned char)(value + 0.5f);
}

int bmp_read(const char *path, BMPImage *image) {
    FILE *fp = NULL;
    int bits_per_pixel;
    int compression;
    size_t data_size;
    int signed_height;

    if (path == NULL || image == NULL) {
        return 1;
    }

    memset(image, 0, sizeof(BMPImage));

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return 2;
    }

    if (fread(image->header, sizeof(unsigned char), 54, fp) != 54) {
        fclose(fp);
        return 3;
    }

    if (image->header[0] != 'B' || image->header[1] != 'M') {
        fclose(fp);
        return 4;
    }

    image->width = *(int *)&image->header[18];
    signed_height = *(int *)&image->header[22];
    bits_per_pixel = *(short *)&image->header[28];
    compression = *(int *)&image->header[30];

    if (image->width <= 0 || signed_height == 0) {
        fclose(fp);
        return 5;
    }

    if (bits_per_pixel != 24 || compression != 0) {
        fclose(fp);
        return 6;
    }

    image->height = signed_height > 0 ? signed_height : -signed_height;
    image->row_padded = (image->width * 3 + 3) & (~3);

    data_size = (size_t)image->row_padded * (size_t)image->height;
    image->data = (unsigned char *)malloc(data_size);
    if (image->data == NULL) {
        fclose(fp);
        return 7;
    }

    if (fread(image->data, sizeof(unsigned char), data_size, fp) != data_size) {
        bmp_free(image);
        fclose(fp);
        return 8;
    }

    fclose(fp);
    return 0;
}

int bmp_write(const char *path, const BMPImage *image) {
    FILE *fp = NULL;
    size_t data_size;

    if (path == NULL || image == NULL || image->data == NULL) {
        return 1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        return 2;
    }

    if (fwrite(image->header, sizeof(unsigned char), 54, fp) != 54) {
        fclose(fp);
        return 3;
    }

    data_size = (size_t)image->row_padded * (size_t)image->height;
    if (fwrite(image->data, sizeof(unsigned char), data_size, fp) != data_size) {
        fclose(fp);
        return 4;
    }

    fclose(fp);
    return 0;
}

int bmp_clone(const BMPImage *src, BMPImage *dst) {
    size_t data_size;

    if (src == NULL || dst == NULL || src->data == NULL) {
        return 1;
    }

    memset(dst, 0, sizeof(BMPImage));
    memcpy(dst->header, src->header, sizeof(src->header));
    dst->width = src->width;
    dst->height = src->height;
    dst->row_padded = src->row_padded;

    data_size = (size_t)src->row_padded * (size_t)src->height;
    dst->data = (unsigned char *)malloc(data_size);
    if (dst->data == NULL) {
        return 2;
    }

    memcpy(dst->data, src->data, data_size);
    return 0;
}

void bmp_free(BMPImage *image) {
    if (image == NULL) {
        return;
    }

    free(image->data);
    image->data = NULL;
    image->width = 0;
    image->height = 0;
    image->row_padded = 0;
    memset(image->header, 0, sizeof(image->header));
}
