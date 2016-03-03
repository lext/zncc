#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

#define NEW_WIDTH 735
#define NEW_HEIGHT 504

unsigned char* OriginalImageL; // Left image
unsigned char* OriginalImageR; // Right image
unsigned char* Disparity;

unsigned char* ImageL; // Left image
unsigned char* ImageR; // Right image

unsigned Error; // Error code
unsigned Width, Height;

unsigned char* resize(unsigned char* image, unsigned w, unsigned h, unsigned w_new, unsigned h_new)
{
	printf("%d %d %d %d\n", w, h, w_new, h_new);
	return NULL;
};



int main(int argc, char** argv)
{
	// Chicking whether images names are given
	if (argc != 3){
		printf("Specify images names!\n");
		return -1;
	}

	// Reading the images to memory
	unsigned w1, h1;
	Error = lodepng_decode32_file(&OriginalImageL, &w1, &h1, argv[1]);
	if(Error) {
		printf("Error in loading of the left image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}

	unsigned w2, h2;
	Error = lodepng_decode32_file(&OriginalImageR, &w2, &h2, argv[2]);
	if(Error){
		printf("Error in loading of the right image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}

	// Checking whether the sizes of images correspond to each other
	if ((w1 != w2) || (h1 != h2)) {
		printf("The sizes of the images do not match!\n");
		return -1;
	}

	Width = w1;
	Height = h1;

	// Resizing the left image and deleting the original array form the memory
	ImageL = resize(OriginalImageL, Width, Height, NEW_WIDTH, NEW_HEIGHT);
	free(OriginalImageL);

	// Resizing the right image and deleting the original array form the memory
	ImageR = resize(OriginalImageR, Width, Height, NEW_WIDTH, NEW_HEIGHT);
	free(OriginalImageR);

	return 0;
}