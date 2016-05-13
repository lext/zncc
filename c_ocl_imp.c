#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "lodepng.h"
#include "cl-helper.h"

/* ---------------- ZNCC parameters ---------------- */

int MAXDISP = 64; // Maximum disparity (downscaled)
int MINDISP = 0;
const int BSX = 21; // Window size on X-axis (width)
const int BSY = 15; // Window size on Y-axis (height)
const int THRESHOLD = 2;// Threshold for cross-checkings
const int NEIBSIZE = 256; // Size of the neighborhood for occlusion-filling


/* ---------------- Macros ---------------- */

// This macros was found somewhere on stackoverflow and it is used to release the memory of
// several arrays
#define FREE_ALL(...) \
do { \
    int i=0;\
    void *pta[] = {__VA_ARGS__}; \
    for(i=0; i < sizeof(pta)/sizeof(void*); i++) \
    { \
        free(pta[i]); \
    }\
} while(0)


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
    
    for (i = 0; i < imsize; i++) {
        if (max-min)
            arr[i] = (uint8_t) (255*(arr[i] - min)/(max-min));
        else
            arr[i] = 0;
    }
}

int32_t main(int32_t argc, char** argv)
{

    uint8_t* OriginalImageL; // Left image
    uint8_t* OriginalImageR; // Right image
    uint8_t* Disparity;

    uint32_t Error; // Error code
    uint32_t Width, Height;
    uint32_t w1, h1;
    uint32_t w2, h2;
    uint32_t imsize;
    clock_t start, end;
    int tmp;
    
    // OpenCL variables
    cl_context ctx;
    cl_command_queue queue;
    cl_int status;
    
	// Checking whether images names are given
	if (argc != 7){
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

	Width = w1/4;
	Height = h1/4;
    imsize = Width*Height;
    char *resize_knl_text = read_file("resize.cl");
    char *zncc_knl_text = read_file("zncc.cl");
    char *cross_check_knl_text = read_file("cross_check.cl");
    char *occlusion_knl_text = read_file("occlusion.cl");
	// Resizing

    /* ---------------- Requesting the device to run the computations ---------------- */

    create_context_on(CHOOSE_INTERACTIVELY, CHOOSE_INTERACTIVELY, 0, &ctx, &queue, 0);
    print_device_info_from_queue(queue);

    /* ---------------- Memory pre-allocation and copying to the device the initial arrays ---------------- */


    // Work group size
    const size_t wgSize[] = {atoi(argv[3]), atoi(argv[4])};
    // Global size
    const size_t globalSize[] = {atoi(argv[5]), atoi(argv[6])};

    const size_t wgSize1D[] = {wgSize[0]*wgSize[1]};
    // Global size
    const size_t globalSize1D[] = {globalSize[0]*globalSize[1]};

    cl_mem dOriginalImageL = clCreateBuffer(ctx, CL_MEM_READ_ONLY, Width*Height*16*4, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");
    CALL_CL_GUARDED(clEnqueueWriteBuffer, (
        queue, dOriginalImageL, CL_TRUE,  0,
        Width*Height*16*4, OriginalImageL,
        0, NULL, NULL));

    cl_mem dOriginalImageR = clCreateBuffer(ctx, CL_MEM_READ_ONLY, Width*Height*16*4, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");
    CALL_CL_GUARDED(clEnqueueWriteBuffer, (
        queue, dOriginalImageR, CL_TRUE,  0,
        Width*Height*16*4, OriginalImageR,
        0, NULL, NULL));

    cl_mem dImageL = clCreateBuffer(ctx, CL_MEM_READ_WRITE, Width*Height, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");

    cl_mem dImageR = clCreateBuffer(ctx, CL_MEM_READ_WRITE, Width*Height, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");

    cl_mem dDisparityLR = clCreateBuffer(ctx, CL_MEM_READ_WRITE, Width*Height, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");

    cl_mem dDisparityRL = clCreateBuffer(ctx, CL_MEM_READ_WRITE, Width*Height, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");
    
    cl_mem dDisparityLRCC = clCreateBuffer(ctx, CL_MEM_READ_WRITE, Width*Height, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");
    
    cl_mem dDisparity = clCreateBuffer(ctx, CL_MEM_READ_WRITE, Width*Height, 0, &status);
    CHECK_CL_ERROR(status, "clCreateBuffer");

    // Creating OpenCL kernels from the corresponding files
    cl_kernel resize_knl = kernel_from_string(ctx, resize_knl_text, "resize", NULL);
    cl_kernel zncc_knl = kernel_from_string(ctx, zncc_knl_text, "zncc", NULL);
    cl_kernel cross_check_knl = kernel_from_string(ctx, cross_check_knl_text, "cross_check", NULL);
    cl_kernel occlusion_knl = kernel_from_string(ctx, occlusion_knl_text, "occlusion", NULL);

    start = clock();

    // Resizing kernel call
    SET_6_KERNEL_ARGS(resize_knl, dOriginalImageL, dOriginalImageR, dImageL, dImageR, w1, h1);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, resize_knl,
             2, NULL, &globalSize, &wgSize,
             0, NULL, NULL));

    // Disparity LR zncc kernel call
    SET_9_KERNEL_ARGS(zncc_knl, dImageL, dImageR, dDisparityLR, Width, Height, BSX, BSY, MINDISP, MAXDISP);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, zncc_knl,
             2, NULL, &globalSize, &wgSize,
             0, NULL, NULL));

    // Disparity RL zncc kernel call
    tmp = MINDISP;
    MINDISP = -MAXDISP;
    MAXDISP = tmp;
    SET_9_KERNEL_ARGS(zncc_knl, dImageR, dImageL, dDisparityRL, Width, Height, BSX, BSY, MINDISP, MAXDISP);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, zncc_knl,
             2, NULL, &globalSize, &wgSize,
             0, NULL, NULL));
    // Cross checking
    SET_5_KERNEL_ARGS(cross_check_knl, dDisparityLR, dDisparityRL, dDisparityLRCC, imsize, THRESHOLD);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, cross_check_knl,
             1, NULL, &globalSize1D, &wgSize1D,
             0, NULL, NULL));
    // occlusion filling
    SET_5_KERNEL_ARGS(occlusion_knl, dDisparityLRCC, dDisparity, Width, Height, NEIBSIZE);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, occlusion_knl,
             2, NULL, &globalSize, &wgSize,
             0, NULL, NULL));
    
    CALL_CL_GUARDED(clFinish, (queue));
    
    Disparity = (uint8_t*) malloc(Width*Height); 
    CALL_CL_GUARDED(clEnqueueReadBuffer, (
        queue, dDisparity, CL_TRUE,  0,
        Width*Height, Disparity,
        0, NULL, NULL));
        
    normalize_dmap(Disparity, Width, Height);
    end = clock();
    
    printf("Elapsed time for calculation of the final disparity map: %.4lf s.\n", (double)(end - start) / CLOCKS_PER_SEC);
    

    Error = lodepng_encode_file("depthmap.png", Disparity, Width, Height, LCT_GREY, 8);
    if(Error){
        printf("Error in saving of the disparity %u: %s\n", Error, lodepng_error_text(Error));
        return -1;
    }   

    
    FREE_ALL(OriginalImageR, OriginalImageL, resize_knl_text);
	return 0;
}
