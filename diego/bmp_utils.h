#ifndef BMP_UTILS_H
#define BMP_UTILS_H

#include <stddef.h>

typedef struct {
    unsigned char header[54];
    int width;
    int height;
    int row_padded;
    unsigned char *data;
} BMPImage;

int bmp_read(const char *path, BMPImage *image);
int bmp_write(const char *path, const BMPImage *image);
int bmp_clone(const BMPImage *src, BMPImage *dst);
void bmp_free(BMPImage *image);

unsigned char to_grayscale(unsigned char b, unsigned char g, unsigned char r);
int make_output_path(const char *name_output, char *out_path, size_t out_size);

#endif
