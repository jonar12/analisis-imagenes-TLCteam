CC = clang
CFLAGS = -Wall -Wextra -std=c11 -O2 -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp

TARGET = para_image_para
SRC = para_image_para.c bmp_utils.c selec_proc.c selec_proc_1.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)