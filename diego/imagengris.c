#include <stdio.h>
#include <stdlib.h>

int main()
{
    FILE *image, *outputImage, *lecturas;
    image = fopen("./input/sample.bmp","rb");          //Imagen original a transformar
    outputImage = fopen("./output/sample.bmp","wb");    //Imagen transformada

    unsigned char r, g, b;               //Pixel

    for(int i=0; i<54; i++) fputc(fgetc(image), outputImage);   //Copia cabecera a nueva imagen (54 primeras lineas en cabecera)
    while(!feof(image)){                                        //Grises -> Obtenemos los bits de la imagen
       b = fgetc(image);
       g = fgetc(image);
       r = fgetc(image);

       unsigned char pixel = 0.21*r+0.72*g+0.07*b;
       fputc(pixel, outputImage);
       fputc(pixel, outputImage);
       fputc(pixel, outputImage);
    }

    fclose(image);
    fclose(outputImage);
    return 0;
}