#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <omp.h>
#include "lodepng.h"

#define MAXDISP 65 // Maximum disparity (downscaled)

#define BSX 15 // Window size on X-axis (width)
#define BSY 15 // Window size on Y-axis (height)

#define THRESHOLD 28 // Threshold for cross-checking

#define NEIBSIZE 50 // Size of the neighborhood for occlusion-filling

uint8_t* resize16gray(const uint8_t* image, uint32_t w, uint32_t h)
{
    /* Downscaling and conversion to 8bit grayscale image */
    
	uint8_t* resized = (uint8_t*) malloc(w*h/16); // Memory pre-allocation for the resized image
	int32_t i, j; // Indices of the resized image 
    int32_t new_w=w/4, new_h=h/4; //  Width and height of the downscaled image
    int32_t orig_i, orig_j; // Indices of the original image
    
    // Iterating through the pixels of the downscaled image
	for (i = 0; i < new_h; i++) {
	    for (j = 0; j < new_w; j++) {
	        // Calculating corresponding indices in the original image
	        orig_i = (4*i-1*(i > 0)); 
	        orig_j = (4*j-1*(j > 0));
	        // Grayscaling
            resized[i*new_w+j] = 0.2126*image[orig_i*(4*w)+4*orig_j]+0.7152*image[orig_i*(4*w)+4*orig_j + 1]+0.0722*image[orig_i*(4*w)+4*orig_j + 2];
		}
	}
	
	return resized;
};

uint8_t* zncc(const uint8_t* left, const uint8_t* right, uint32_t w, uint32_t h, uint32_t bsx, uint32_t bsy, uint32_t maxd)
{
    /* Disparity map computation */
    int32_t imsize = w*h; // Size of the image
    int32_t bsize = bsx*bsy; // Block size

    uint8_t* dmap = (uint8_t*) malloc(imsize); // Memory allocation for the disparity map
    int32_t i, j; // Indices for rows and colums respectively
    int32_t i_b, j_b; // Indices within the block
    int32_t ind_l, ind_r; // Indices of block values within the whole image
    uint8_t d; // Disparity value
    double_t cl, cr; // centered values of a pixel in the left and right images;
    
    double_t lbmean, rbmean; // Blocks means for left and right images
    double_t lbstd, rbstd; // Left block std, Right block std
    double_t current_score; // Current ZNCC value
    
    uint8_t best_d;
    double_t best_score;
    
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            // Searching for the best d for the current pixel
            best_d = maxd;
            best_score = -1;
            for (d = 0; d <= maxd; d++) {
                // Calculating the blocks' means
                lbmean = 0;
                rbmean = 0;
                for (i_b = 0; i_b < bsy; i_b++) {
                    for (j_b = 0; j_b < bsx; j_b++) {
                        // Calculatiing indices of the block within the whole image
                        ind_l = (i+i_b)*w + (j+j_b);
                        ind_r = (i+i_b)*w + (j+j_b-d);
                        
                        // Artificial zero-padding
                        if ((ind_l < 0) || (ind_l >= imsize))
                            continue;
                        if ((ind_r < 0) || (ind_r >= imsize))
                            continue;
                        // Updating the blocks' means
                        lbmean += left[ind_l];
                        rbmean += right[ind_r];
                    }
                }
                lbmean /= bsize;
                rbmean /= bsize;
                
                // Calculating ZNCC for given value of d
                lbstd = 0;
                rbstd = 0;
                current_score = 0;
                
                // Calculating the nomentaor and the standard deviations for the denominator
                for (i_b = 0; i_b < bsy; i_b++) {
                    for (j_b = 0; j_b < bsx; j_b++) {
                        // Calculatiing indices of the block within the whole image
                        ind_l = (i+i_b)*w + (j+j_b);
                        ind_r = (i+i_b)*w + (j+j_b-d);
                        
                        // Artificial zero-padding
                        if ((ind_l < 0) || (ind_l >= imsize)) {
                            //printf("Weird_1\n");
                            continue;
                        }
                        if ((ind_r < 0) || (ind_r >= imsize)) {
                            //printf("Weird_2! ind_r=%d, d=%d, w=%d\n", ind_r, d, w);
                            continue;
                        }
                            
                        cl = left[ind_l] - lbmean;
                        cr = right[ind_r] - rbmean;
                        lbstd += cl*cl;
                        rbstd += cr*cr;
                        current_score += cl*cr;
                    }
                }
                // Normalizing the denominator
                current_score /= sqrt(lbstd)*sqrt(rbstd);
                // Selecting the best disparity
                if (current_score > best_score) {
                    best_score = current_score;
                    best_d = d;
                }
            }
            dmap[i*w+j] = best_d;
        } 
    }
    
    return dmap;
}

void normalize_dmap(uint8_t* arr, uint32_t w, uint32_t h)
{
    uint8_t max = 0;
    uint8_t min = UCHAR_MAX;
    int32_t imsize = w*h;
    uint32_t i;
    for (i = 0; i < imsize; i++) {
        if (arr[i] > max)
            max = arr[i];
        if (arr[i] < min)
            min = arr[i];
    }
    #pragma omp parallel for
    for (i = 0; i < imsize; i++) {
        arr[i] = (uint8_t) (255*(arr[i] - min)/max);
    }
}

uint8_t* cross_checking(const uint8_t* map1, const uint8_t* map2, uint32_t imsize, uint32_t threshold) {
    uint8_t* map = (uint8_t*) malloc(imsize); 
    uint32_t idx;
    #pragma omp parallel for
    for (idx = 0; idx < imsize; idx++) {
        if (abs(map1[idx] - map2[idx]) > threshold)
            map[idx] = 0;
        else
            map[idx] = map1[idx];
    }
    return map;
}


uint8_t* oclusion_filling(uint8_t* map, uint32_t w, uint32_t h, uint32_t bsx, uint32_t bsy) {
    int32_t imsize = w*h; // Size of the image

    uint8_t* result = (uint8_t*) malloc(imsize);
    int32_t i, j; // Indices for rows and colums respectively
    int32_t i_b, j_b; // Indices within the block
    int32_t ind_neib; // Index in the nighbourhood
    bool stop; // Stop flag for nearest neighbor interpolation
    
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            // If the value of the pixel is zero, perform the occlusion filling by nearest neighbour interpolation
            result[i*w+j] = map[i*w+j];
            if(map[i*w+j] == 0) {
                stop = false;
                for (i_b = 0; i_b < bsy && !stop; i_b++) {
                    for (j_b = 0; j_b < bsx && !stop; j_b++) {
                        // Calculatiing indices of the block within the whole image
                        ind_neib = (i+i_b)*w + (j+j_b);
                        // Artificial zero-padding
                        if ((ind_neib < 0) || (ind_neib >= imsize) || ind_neib == i*w+j) {
                            continue;
                        }
                        // If we meet a nonzero pixel, we interpolate and quite from this loop
                        if(map[ind_neib] != 0) {
                            result[i*w+j] = map[ind_neib];
                            stop = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}

int32_t main(int32_t argc, char** argv)
{
    uint8_t* OriginalImageL; // Left image
    uint8_t* OriginalImageR; // Right image
    uint8_t* DisparityLR;
    uint8_t* DisparityRL;
    uint8_t* Disparity;

    uint8_t* ImageL; // Left image
    uint8_t* ImageR; // Right image

    uint32_t Error; // Error code
    uint32_t Width, Height;
    uint32_t w1, h1;
    uint32_t w2, h2;
    
	// Chicking whether images names are given
	if (argc != 3){
		printf("Specify images names!\n");
		return -1;
	}

	// Reading the images into memory
	Error = lodepng_decode32_file(&OriginalImageL, &w1, &h1, argv[1]);
	if(Error) {
		printf("Error in loading of the left image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}

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
	// Resizing
    ImageL = resize16gray(OriginalImageL, Width, Height); // Left Image
    ImageR = resize16gray(OriginalImageR, Width, Height); // Right Image
    Width = Width/4;
    Height = Height/4;
    
    // Calculating the disparity maps
    printf("Computing maps with zncc...\n");
    DisparityLR = zncc(ImageL, ImageR, Width, Height, BSX, BSY, MAXDISP);
    DisparityRL = zncc(ImageR, ImageL, Width, Height, BSX, BSY, MAXDISP);
    // Cross-checking
    printf("Performing cross-checking...\n");
    Disparity = cross_checking(DisparityLR, DisparityRL,  Width * Height, THRESHOLD);
    // Occlusion-filling
    printf("Performing occlusion-filling...\n");
    Disparity = oclusion_filling(Disparity, Width, Height, NEIBSIZE, NEIBSIZE);
    
    // Normalization
    printf("Performing maps normalization...\n");
    normalize_dmap(DisparityLR, Width, Height);
    normalize_dmap(DisparityRL, Width, Height);
    normalize_dmap(Disparity, Width, Height);
	
	// Saving the results
    Error = lodepng_encode_file("resized_left.png", ImageL, Width, Height, LCT_GREY, 8);
	if(Error){
		printf("Error in saving of the left image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	
	Error = lodepng_encode_file("resized_right.png", ImageR, Width, Height, LCT_GREY, 8);
	if(Error){
		printf("Error in saving of the right image %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	
	Error = lodepng_encode_file("depthmap_no_post_procLR.png", DisparityLR, Width, Height, LCT_GREY, 8);
	if(Error){
		printf("Error in saving of the disparity %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	
	Error = lodepng_encode_file("depthmap_no_post_procRL.png", DisparityRL, Width, Height, LCT_GREY, 8);
	if(Error){
		printf("Error in saving of the disparity %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	Error = lodepng_encode_file("depthmap.png", Disparity, Width, Height, LCT_GREY, 8);
	if(Error){
		printf("Error in saving of the disparity %u: %s\n", Error, lodepng_error_text(Error));
		return -1;
	}
	
	
	free(OriginalImageL);
	free(OriginalImageR);
	free(ImageL);
	free(ImageR);
	return 0;
}
