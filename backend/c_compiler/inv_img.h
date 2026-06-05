// Las transformaciones separan la E/S (lectura/escritura de archivos, que en un
// cluster MPI viaja por red/NFS) del cómputo puro. Solo se cronometra el cómputo
// en memoria; el tiempo devuelto en *compute_seconds NO incluye tiempo de red.

// ── Gris + espejo vertical ──
extern void inv_img_grey_vertical(char *name, char *path, double *compute_seconds) {
    if (compute_seconds) *compute_seconds = 0.0;

    char output_path[100] = "../img/";
    strcat(output_path, name);

    FILE *image = fopen(path, "rb");
    if (!image) { printf("Error abriendo archivos.\n"); return; }

    // ── Lectura (E/S de red, NO cronometrada) ──
    unsigned char xx[54];
    if (fread(xx, 1, 54, image) != 54) { fclose(image); return; }
    long ancho = (long)xx[20]*65536 + (long)xx[19]*256 + (long)xx[18];
    long alto  = (long)xx[24]*65536 + (long)xx[23]*256 + (long)xx[22];
    long npix  = alto * ancho;

    unsigned char *raw = (unsigned char*)malloc(npix * 3);
    unsigned char *grey = (unsigned char*)malloc(npix);
    unsigned char *out  = (unsigned char*)malloc(npix * 3);
    fread(raw, 1, npix * 3, image);
    fclose(image);

    // ── Cómputo (en memoria, cronometrado) ──
    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(static)
    for (long p = 0; p < npix; p++) {
        unsigned char b = raw[p*3];
        unsigned char g = raw[p*3+1];
        unsigned char r = raw[p*3+2];
        grey[p] = 0.21*r + 0.72*g + 0.07*b;
    }
    #pragma omp parallel for schedule(static)
    for (long row = 0; row < alto; row++) {
        for (long col = 0; col < ancho; col++) {
            long src = row * ancho + (ancho - 1 - col);
            long o   = (row * ancho + col) * 3;
            out[o] = out[o+1] = out[o+2] = grey[src];
        }
    }
    if (compute_seconds) *compute_seconds = omp_get_wtime() - t0;

    // ── Escritura (E/S de red, NO cronometrada) ──
    FILE *outputImage = fopen(output_path, "wb");
    if (outputImage) {
        fwrite(xx, 1, 54, outputImage);
        fwrite(out, 1, npix * 3, outputImage);
        fclose(outputImage);
    }
    free(raw); free(grey); free(out);
}

// ── Gris + espejo horizontal ──
extern void inv_img_grey_horizontal(char *name, char *path, double *compute_seconds) {
    if (compute_seconds) *compute_seconds = 0.0;

    char output_path[100] = "../img/";
    strcat(output_path, name);

    FILE *image = fopen(path, "rb");
    if (!image) { printf("Error abriendo archivos.\n"); return; }

    // ── Lectura (E/S de red, NO cronometrada) ──
    unsigned char xx[54];
    if (fread(xx, 1, 54, image) != 54) { fclose(image); return; }
    long ancho = (long)xx[20]*65536 + (long)xx[19]*256 + (long)xx[18];
    long alto  = (long)xx[24]*65536 + (long)xx[23]*256 + (long)xx[22];
    long npix  = alto * ancho;

    unsigned char *raw = (unsigned char*)malloc(npix * 3);
    unsigned char *grey = (unsigned char*)malloc(npix);
    unsigned char *out  = (unsigned char*)malloc(npix * 3);
    fread(raw, 1, npix * 3, image);
    fclose(image);

    // ── Cómputo (en memoria, cronometrado) ──
    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(static)
    for (long p = 0; p < npix; p++) {
        unsigned char b = raw[p*3];
        unsigned char g = raw[p*3+1];
        unsigned char r = raw[p*3+2];
        grey[p] = 0.21*r + 0.72*g + 0.07*b;
    }
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < npix; i++) {
        out[i*3] = out[i*3+1] = out[i*3+2] = grey[npix - 1 - i];
    }
    if (compute_seconds) *compute_seconds = omp_get_wtime() - t0;

    // ── Escritura (E/S de red, NO cronometrada) ──
    FILE *outputImage = fopen(output_path, "wb");
    if (outputImage) {
        fwrite(xx, 1, 54, outputImage);
        fwrite(out, 1, npix * 3, outputImage);
        fclose(outputImage);
    }
    free(raw); free(grey); free(out);
}

// ── Color + espejo vertical ──
extern void inv_img_color_vertical(char *name, char *path, double *compute_seconds) {
    if (compute_seconds) *compute_seconds = 0.0;

    char output_path[100] = "../img/";
    strcat(output_path, name);

    FILE *image = fopen(path, "rb");
    if (!image) { printf("Error abriendo archivos.\n"); return; }

    // ── Lectura (E/S de red, NO cronometrada) ──
    unsigned char xx[54];
    if (fread(xx, 1, 54, image) != 54) { fclose(image); return; }
    long ancho = (long)xx[20]*65536 + (long)xx[19]*256 + (long)xx[18];
    long alto  = (long)xx[24]*65536 + (long)xx[23]*256 + (long)xx[22];
    long npix  = alto * ancho;

    unsigned char *raw = (unsigned char*)malloc(npix * 3);
    unsigned char *out = (unsigned char*)malloc(npix * 3);
    fread(raw, 1, npix * 3, image);
    fclose(image);

    // ── Cómputo (en memoria, cronometrado) ──
    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(static)
    for (long row = 0; row < alto; row++) {
        long o = row * ancho * 3;
        for (long col = 0; col < ancho; col++) {
            long idx = (row * ancho + (ancho - 1 - col)) * 3;
            out[o++] = raw[idx];     // B
            out[o++] = raw[idx+1];   // G
            out[o++] = raw[idx+2];   // R
        }
    }
    if (compute_seconds) *compute_seconds = omp_get_wtime() - t0;

    // ── Escritura (E/S de red, NO cronometrada) ──
    FILE *outputImage = fopen(output_path, "wb");
    if (outputImage) {
        fwrite(xx, 1, 54, outputImage);
        fwrite(out, 1, npix * 3, outputImage);
        fclose(outputImage);
    }
    free(raw); free(out);
}

// ── Color + espejo horizontal ──
extern void inv_img_color_horizontal(char *name, char *path, double *compute_seconds) {
    if (compute_seconds) *compute_seconds = 0.0;

    char output_path[100] = "../img/";
    strcat(output_path, name);

    FILE *image = fopen(path, "rb");
    if (!image) { printf("Error abriendo archivos.\n"); return; }

    // ── Lectura (E/S de red, NO cronometrada) ──
    unsigned char xx[54];
    if (fread(xx, 1, 54, image) != 54) { fclose(image); return; }
    long ancho = (long)xx[20]*65536 + (long)xx[19]*256 + (long)xx[18];
    long alto  = (long)xx[24]*65536 + (long)xx[23]*256 + (long)xx[22];
    long npix  = alto * ancho;

    unsigned char *raw = (unsigned char*)malloc(npix * 3);
    unsigned char *out = (unsigned char*)malloc(npix * 3);
    fread(raw, 1, npix * 3, image);
    fclose(image);

    // ── Cómputo (en memoria, cronometrado) ──
    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < npix; i++) {
        long s = (npix - 1 - i) * 3;
        out[i*3]   = raw[s];     // B
        out[i*3+1] = raw[s+1];   // G
        out[i*3+2] = raw[s+2];   // R
    }
    if (compute_seconds) *compute_seconds = omp_get_wtime() - t0;

    // ── Escritura (E/S de red, NO cronometrada) ──
    FILE *outputImage = fopen(output_path, "wb");
    if (outputImage) {
        fwrite(xx, 1, 54, outputImage);
        fwrite(out, 1, npix * 3, outputImage);
        fclose(outputImage);
    }
    free(raw); free(out);
}
