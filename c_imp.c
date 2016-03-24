#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

unsigned char* OriginalImageL; // Left image
unsigned char* OriginalImageR; // Right image
unsigned char* Disparity;

unsigned char* ImageL; // Left image
unsigned char* ImageR; // Right image

unsigned Error; // Error code
unsigned Width, Height;

unsigned char* resize16(unsigned char* image, unsigned w, unsigned h)
{
	unsigned char* resized = (unsigned char*) malloc(3*w*h/16);
	int i, j;
	int new_w, new_h;
    int orig_i, orig_j;
    new_h = h/4;
    new_w = w/4;
    
	for (i = 0; i < new_h; i++) {
	    for(j = 0; j < new_w; j++) {
	        orig_i = (4*i-1*(i > 0));
	        orig_j = (4*j-1*(j > 0));
	        
	        resized[i*(3*new_w)+3*j] = image[orig_i*(4*w)+4*orig_j];
	        resized[i*(3*new_w)+3*j+1] = image[orig_i*(4*w)+4*orig_j+1];
	        resized[i*(3*new_w)+3*j+2] = image[orig_i*(4*w)+4*orig_j+2];
		}
	}

	return resized;
};

unsigned char* resize16gray(unsigned char* image, unsigned w, unsigned h)
{
	unsigned char* resized = (unsigned char*) malloc(w*h/16);
	int i, j;
	int new_w, new_h;
    int orig_i, orig_j;
    new_h = h/4;
    new_w = w/4;
    
	for (i = 0; i < new_h; i++) {
	    for(j = 0; j < new_w; j++) {
	        orig_i = (4*i-1*(i > 0));
	        orig_j = (4*j-1*(j > 0));
	        resized[i*new_w+j] = 0.2126*image[orig_i*(4*w)+4*orig_j]+0.7152*image[orig_i*(4*w)+4*orig_j]+0.0722*image[orig_i*(4*w)+4*orig_j];
		}
	}

	return resized;
};

//unsigned char* rgb2gray(unsigned char* image, unsigned w, unsigned h) {

//}

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

    /* Testing just resize algorithm
    
	// Resizing the left image and deleting the original array form the memory
	ImageL = resize16(OriginalImageL, Width, Height);
	
	Error = lodepng_encode24_file("resized_left.png", ImageL, Width/4, Height/4);
	if(Error){
		printf("Error in saving of the left image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}

	// Resizing the right image and deleting the original array form the memory
	ImageR = resize16(OriginalImageR, Width, Height);

	Error = lodepng_encode24_file("resized_right.png", ImageR, Width/4, Height/4);
	if(Error){
		printf("Error in saving of the right image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	*/
	
	/* Testing rgb2gray resize algorithm */
	ImageL = resize16gray(OriginalImageL, Width, Height);
    Error = lodepng_encode_file("resized_left.png", ImageL, Width/4, Height/4, LCT_GREY, 8);
	if(Error){
		printf("Error in saving of the left image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	
    ImageR = resize16gray(OriginalImageR, Width, Height);

	Error = lodepng_encode_file("resized_right.png", ImageR, Width/4, Height/4, LCT_GREY, 8);
	if(Error){
		printf("Error in saving of the right image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	
	free(OriginalImageL);
	free(OriginalImageR);
	free(ImageL);
	free(ImageR);
	return 0;
}
