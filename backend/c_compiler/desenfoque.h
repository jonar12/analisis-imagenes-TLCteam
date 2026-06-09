// El desenfoque separa la E/S (lectura/escritura de archivos, que en un cluster
// MPI viaja por red/NFS) del cómputo puro. Solo se cronometra el cómputo en
// memoria; el tiempo devuelto en *compute_seconds NO incluye tiempo de red.
extern void desenfoque_color(const char* input_path, const char* name_output,
                             int kernel_size, double *compute_seconds) {
    if (compute_seconds) *compute_seconds = 0.0;

    char output_path[100] = "../img/";
    strcat(output_path, name_output);

    FILE *image = fopen(input_path, "rb");
    if (!image) { printf("Error abriendo archivos.\n"); return; }

    // ── Lectura (E/S de red, NO cronometrada) ──
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, image);

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int row_padded = (width * 3 + 3) & (~3);

    unsigned char** input_rows = (unsigned char**)malloc(height * sizeof(unsigned char*));
    unsigned char** output_rows = (unsigned char**)malloc(height * sizeof(unsigned char*));
    unsigned char** temp_rows = (unsigned char**)malloc(height * sizeof(unsigned char*));

    for (int i = 0; i < height; i++) {
        input_rows[i] = (unsigned char*)malloc(row_padded);
        output_rows[i] = (unsigned char*)malloc(row_padded);
        temp_rows[i] = (unsigned char*)malloc(row_padded);
        fread(input_rows[i], sizeof(unsigned char), row_padded, image);
    }
    fclose(image);

    int k = kernel_size / 2;

    // ── Cómputo (en memoria, cronometrado) ──
    double t0 = omp_get_wtime();

    // Desenfoque de caja separable con ventana deslizante (suma corriente):
    // cada paso es O(width*height), INDEPENDIENTE del tamaño del kernel.
    // Resultado idéntico al box blur clásico con borde recortado (divide entre
    // el número real de muestras válidas). Filas/columnas son independientes,
    // así que se paralelizan con OpenMP.

    // Paso intermedio: desenfoque horizontal (cada fila es independiente)
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; y++) {
        const unsigned char *in = input_rows[y];
        unsigned char *out = temp_rows[y];
        long sumB = 0, sumG = 0, sumR = 0;
        int wl = 0;                          // ventana inclusiva [wl, wr]
        int wr = (k < width - 1) ? k : width - 1;
        for (int nx = wl; nx <= wr; nx++) {
            int idx = nx * 3;
            sumB += in[idx]; sumG += in[idx + 1]; sumR += in[idx + 2];
        }
        for (int x = 0; x < width; x++) {
            int nl = x - k; if (nl < 0) nl = 0;
            int nr = x + k; if (nr > width - 1) nr = width - 1;
            while (wl < nl) { int idx = wl * 3; sumB -= in[idx]; sumG -= in[idx+1]; sumR -= in[idx+2]; wl++; }
            while (wr < nr) { wr++; int idx = wr * 3; sumB += in[idx]; sumG += in[idx+1]; sumR += in[idx+2]; }
            int count = nr - nl + 1;
            int index = x * 3;
            out[index + 0] = (unsigned char)(sumB / count);
            out[index + 1] = (unsigned char)(sumG / count);
            out[index + 2] = (unsigned char)(sumR / count);
        }
        // Copiar padding sin modificar
        for (int p = width * 3; p < row_padded; p++) out[p] = in[p];
    }

    // Paso final: desenfoque vertical (cada columna es independiente)
    #pragma omp parallel for schedule(static)
    for (int x = 0; x < width; x++) {
        int idx = x * 3;
        long sumB = 0, sumG = 0, sumR = 0;
        int wt = 0;                          // ventana inclusiva [wt, wb] sobre filas
        int wb = (k < height - 1) ? k : height - 1;
        for (int ny = wt; ny <= wb; ny++) {
            sumB += temp_rows[ny][idx]; sumG += temp_rows[ny][idx+1]; sumR += temp_rows[ny][idx+2];
        }
        for (int y = 0; y < height; y++) {
            int nt = y - k; if (nt < 0) nt = 0;
            int nb = y + k; if (nb > height - 1) nb = height - 1;
            while (wt < nt) { sumB -= temp_rows[wt][idx]; sumG -= temp_rows[wt][idx+1]; sumR -= temp_rows[wt][idx+2]; wt++; }
            while (wb < nb) { wb++; sumB += temp_rows[wb][idx]; sumG += temp_rows[wb][idx+1]; sumR += temp_rows[wb][idx+2]; }
            int count = nb - nt + 1;
            output_rows[y][idx + 0] = (unsigned char)(sumB / count);
            output_rows[y][idx + 1] = (unsigned char)(sumG / count);
            output_rows[y][idx + 2] = (unsigned char)(sumR / count);
        }
    }
    // Copiar padding sin modificar (después del paso vertical)
    for (int y = 0; y < height; y++)
        for (int p = width * 3; p < row_padded; p++)
            output_rows[y][p] = temp_rows[y][p];

    if (compute_seconds) *compute_seconds = omp_get_wtime() - t0;

    // ── Escritura (E/S de red, NO cronometrada) ──
    FILE *outputImage = fopen(output_path, "wb");
    if (outputImage) {
        fwrite(header, sizeof(unsigned char), 54, outputImage);
        for (int i = 0; i < height; i++) {
            fwrite(output_rows[i], sizeof(unsigned char), row_padded, outputImage);
        }
        fclose(outputImage);
    }

    for (int i = 0; i < height; i++) {
        free(input_rows[i]);
        free(temp_rows[i]);
        free(output_rows[i]);
    }
    free(input_rows);
    free(temp_rows);
    free(output_rows);
}
