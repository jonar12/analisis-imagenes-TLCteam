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

    // Paso intermedio: desenfoque horizontal
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sumB = 0, sumG = 0, sumR = 0, count = 0;

            for (int dx = -k; dx <= k; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < width) {
                    int idx = nx * 3;
                    sumB += input_rows[y][idx + 0];
                    sumG += input_rows[y][idx + 1];
                    sumR += input_rows[y][idx + 2];
                    count++;
                }
            }

            int index = x * 3;
            temp_rows[y][index + 0] = sumB / count;
            temp_rows[y][index + 1] = sumG / count;
            temp_rows[y][index + 2] = sumR / count;
        }

        // Copiar padding sin modificar
        for (int p = width * 3; p < row_padded; p++) {
            temp_rows[y][p] = input_rows[y][p];
        }
    }

    // Paso final: desenfoque vertical
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sumB = 0, sumG = 0, sumR = 0, count = 0;

            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < height) {
                    int idx = x * 3;
                    sumB += temp_rows[ny][idx + 0];
                    sumG += temp_rows[ny][idx + 1];
                    sumR += temp_rows[ny][idx + 2];
                    count++;
                }
            }

            int index = x * 3;
            output_rows[y][index + 0] = sumB / count;
            output_rows[y][index + 1] = sumG / count;
            output_rows[y][index + 2] = sumR / count;
        }

        // Copiar padding sin modificar
        for (int p = width * 3; p < row_padded; p++) {
            output_rows[y][p] = temp_rows[y][p];
        }
    }

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
