// Utilizamos malloc para saber que pixel estamos trabajando a partir del cálculo de las dimensiones de la imagen
extern void inv_img_grey_vertical(char *name, char *path) {
    FILE *image, *outputImage;
    char output_path[100] = "../img/";
    strcat(output_path, name);
    image = fopen(path,"rb");
    outputImage = fopen(output_path,"wb");

    if (!image || !outputImage) {
        printf("Error abriendo archivos.\n");
        return;
    }
    long ancho;
    long alto;
    unsigned char r, g, b;               //Pixel
    unsigned char* ptr;

    unsigned char xx[54];
    int cuenta = 0;
    for(int i=0; i<54; i++) {
      xx[i] = fgetc(image);
      fputc(xx[i], outputImage);   //Copia cabecera a nueva imagen
    }
    ancho = (long)xx[20]*65536+(long)xx[19]*256+(long)xx[18];
    alto = (long)xx[24]*65536+(long)xx[23]*256+(long)xx[22];
    // printf("largo img %li\n",alto);
    // printf("ancho img %li\n",ancho);

    ptr = (unsigned char*)malloc(alto*ancho*3*sizeof(unsigned char));

    while(!feof(image)){
      b = fgetc(image);
      g = fgetc(image);
      r = fgetc(image);


      unsigned char pixel = 0.21*r+0.72*g+0.07*b;

      ptr[cuenta] = pixel; //b
      ptr[cuenta+1] = pixel; //g
      ptr[cuenta+2] = pixel; //r

      cuenta+=3;
    }                                        //Grises
    // printf("%d\n",cuenta);
    for (int row = 0; row < alto; row++) {
        for (int col = ancho - 1; col >= 0; col--) {  
            int idx = (row * ancho + col) * 3;
            fputc(ptr[idx], outputImage);
            fputc(ptr[idx], outputImage);
            fputc(ptr[idx], outputImage);
        }
    }
    free(ptr);
    fclose(image);
    fclose(outputImage);
}

// Utilizamos malloc para saber que pixel estamos trabajando a partir del cálculo de las dimensiones de la imagen
extern void inv_img_grey_horizontal(char *name, char *path) {
    FILE *image, *outputImage;
    char output_path[100] = "../img/";
    strcat(output_path, name);
    image = fopen(path,"rb");
    outputImage = fopen(output_path,"wb");

    if (!image || !outputImage) {
        printf("Error abriendo archivos.\n");
        return;
    }
    long ancho;
    long alto;
    unsigned char r, g, b;               //Pixel
    unsigned char* ptr;

    unsigned char xx[54];
    int cuenta = 0;
    for(int i=0; i<54; i++) {
      xx[i] = fgetc(image);
      fputc(xx[i], outputImage);   //Copia cabecera a nueva imagen
    }
    ancho = (long)xx[20]*65536+(long)xx[19]*256+(long)xx[18];
    alto = (long)xx[24]*65536+(long)xx[23]*256+(long)xx[22];
    // printf("largo img %li\n",alto);
    // printf("ancho img %li\n",ancho);

    ptr = (unsigned char*)malloc(alto*ancho*3*sizeof(unsigned char));

    while(!feof(image)){
      b = fgetc(image);
      g = fgetc(image);
      r = fgetc(image);


      unsigned char pixel = 0.21*r+0.72*g+0.07*b;

      ptr[cuenta] = pixel; //b
      ptr[cuenta+1] = pixel; //g
      ptr[cuenta+2] = pixel; //r


      cuenta++;

    }                                        //Grises
    // printf("%d\n",cuenta);
    cuenta = 0;
    for (int i = 0; i < alto*ancho; ++i) {
      // fputc(ptr[i], outputImage);
      // fputc(ptr[i+1], outputImage);
      // fputc(ptr[i+2], outputImage);
      fputc(ptr[(ancho * alto) - i], outputImage);
      fputc(ptr[(ancho * alto) - i], outputImage);
      fputc(ptr[(ancho * alto) - i], outputImage);
      cuenta++;
      if (cuenta == 0){
        cuenta = ancho;
      }
    }
    free(ptr);
    fclose(image);
    fclose(outputImage);
}

// Utilizamos malloc para saber que pixel estamos trabajando a partir del cálculo de las dimensiones de la imagen
extern void inv_img_color_vertical(char *name, char *path) {
    FILE *image, *outputImage;
    char output_path[100] = "../img/";
    strcat(output_path, name);
    image = fopen(path,"rb");
    outputImage = fopen(output_path,"wb");

    if (!image || !outputImage) {
        printf("Error abriendo archivos.\n");
        return;
    }
    long ancho;
    long alto;
    unsigned char r, g, b;               //Pixel
    unsigned char* ptr;

    unsigned char xx[54];
    int cuenta = 0;
    for(int i=0; i<54; i++) {
      xx[i] = fgetc(image);
      fputc(xx[i], outputImage);   //Copia cabecera a nueva imagen
    }
    ancho = (long)xx[20]*65536+(long)xx[19]*256+(long)xx[18];
    alto = (long)xx[24]*65536+(long)xx[23]*256+(long)xx[22];
    // printf("largo img %li\n",alto);
    // printf("ancho img %li\n",ancho);

    ptr = (unsigned char*)malloc(alto*ancho*3*sizeof(unsigned char));

    while(!feof(image)){
      b = fgetc(image);
      g = fgetc(image);
      r = fgetc(image);


      // unsigned char pixel = 0.21*r+0.72*g+0.07*b;

      ptr[cuenta] = b; //b
      ptr[cuenta+1] = g; //g
      ptr[cuenta+2] = r; //r

      cuenta+=3;
    }                                        //Grises
    // printf("%d\n",cuenta);
    for (int row = 0; row < alto; row++) {
      for (int col = 0; col < ancho; col++) {
          int src_col = ancho - 1 - col; 
          int idx = (row * ancho + src_col) * 3;
          fputc(ptr[idx], outputImage);     // B
          fputc(ptr[idx+1], outputImage);   // G
          fputc(ptr[idx+2], outputImage);   // R
        }
    }
    free(ptr);
    fclose(image);
    fclose(outputImage);
}


extern void inv_img_color_horizontal(char *name, char *path) {
    FILE *image, *outputImage;
    char output_path[100] = "../img/";
    strcat(output_path, name);
    image = fopen(path,"rb");
    outputImage = fopen(output_path,"wb");

    if (!image || !outputImage) {
        printf("Error abriendo archivos.\n");
        return;
    }
    long ancho;
    long alto;
    unsigned char r, g, b;               //Pixel
    unsigned char* ptr;

    unsigned char xx[54];
    int cuenta = 0;
    for(int i=0; i<54; i++) {
      xx[i] = fgetc(image);
      fputc(xx[i], outputImage);   //Copia cabecera a nueva imagen
    }
    ancho = (long)xx[20]*65536+(long)xx[19]*256+(long)xx[18];
    alto = (long)xx[24]*65536+(long)xx[23]*256+(long)xx[22];
    // printf("largo img %li\n",alto);
    // printf("ancho img %li\n",ancho);

    ptr = (unsigned char*)malloc(alto*ancho*3*sizeof(unsigned char));

    while(!feof(image)){
      b = fgetc(image);
      g = fgetc(image);
      r = fgetc(image);


      ptr[cuenta*3]   = b;
      ptr[cuenta*3+1] = g;
      ptr[cuenta*3+2] = r;

      cuenta++;

    }
    // printf("%d\n",cuenta);
    cuenta = 0;
    for (int i = 0; i < alto*ancho; ++i) {
      // fputc(ptr[i], outputImage);
      // fputc(ptr[i+1], outputImage);
      // fputc(ptr[i+2], outputImage);
      fputc(ptr[((ancho * alto) - i) * 3],     outputImage);
      fputc(ptr[((ancho * alto) - i) * 3 + 1], outputImage);
      fputc(ptr[((ancho * alto) - i) * 3 + 2], outputImage);
      cuenta++;
      if (cuenta == 0){
        cuenta = ancho;
      }
    }
    free(ptr);
    fclose(image);
    fclose(outputImage);
}
