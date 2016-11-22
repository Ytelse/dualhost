#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define INPUT_FP	"mnist-ubyte-no-header"
#define OUTPUT_FP	"bit_mnist_images"

#define IMG_X 28
#define IMG_Y 28

#define THRESHOLD 40

#define NUM_IMG 70000
#define IMG_SET_SIZE 28*28*70000

typedef unsigned char byte_t;

void convert_and_write(FILE* fin, FILE* fout) {
	unsigned int pixel;
	byte_t byte = 0;
	for (int i = 0; i < IMG_SET_SIZE; i+=8) {
		byte = 0;
		for (int j = 0; j < 8; j++) {
			pixel = getc(fin);
			if (pixel >= THRESHOLD) {
				pixel = 1;
			} else {
				pixel = 0;
			}

			byte |= (pixel << (7-i));
		}
		putc(byte, fout);
	}
}

int main(void) {

	FILE* fin, *fout;

	fin = fopen(INPUT_FP, "rb");
	fout = fopen(OUTPUT_FP, "wb");

	assert(fin);
	assert(fout);

	convert_and_write(fin, fout);

	fclose(fin);
	fclose(fout);

	return 0;
}