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
const uint32_t BSX = 21; // Window size on X-axis (width)
const uint32_t BSY = 15; // Window size on Y-axis (height)
const int THRESHOLD = 2;// Threshold for cross-checkings
const int NEIBSIZE = 256; // Size of the neighborhood for occlusion-filling
const uint32_t BSIZE = 315;

cl_image_format format = { CL_RGBA, CL_UNSIGNED_INT8 };
cl_image_desc desc;
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
    
    struct timespec start, finish;
    double elapsed;

    int tmp;
    const char* OUT_DEVINFO = getenv("OUT_DEVINFO");
    const char* CL_PLAT = getenv("CL_PLAT");
    const char* CL_DEVICE = getenv("CL_DEVICE");

    
    // OpenCL variables
    cl_context ctx;
    cl_command_queue queue;
    cl_int status;
    
	// Checking whether images names are given
	if (argc != 7){
        printf("Correct number of args! Expected 6, given %d.\n", argc-1);
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
    if(!CL_PLAT || !CL_DEVICE)
        create_context_on(CHOOSE_INTERACTIVELY, CHOOSE_INTERACTIVELY, 0, &ctx, &queue, 1);
    else
        create_context_on(CL_PLAT, CL_DEVICE, 0, &ctx, &queue, 1);
    if (OUT_DEVINFO)
        print_device_info_from_queue(queue);

    /* ---------------- Memory pre-allocation and copying to the device the initial arrays ---------------- */


    // Work group size
    const size_t wgSize[] = {atoi(argv[3]), atoi(argv[4])};

    // Global size
    const size_t globalSize[] = {atoi(argv[5]), atoi(argv[6])};

    const size_t wgSize1D[] = {wgSize[0]*wgSize[1]};
    // Global size
    const size_t globalSize1D[] = {globalSize[0]*globalSize[1]};


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


    Disparity = (uint8_t*) malloc(Width*Height); 
     
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = w1;
    desc.image_height = h1;
    desc.image_depth = 8;
    desc.image_row_pitch = w1 * 4;
    
    
    
    cl_mem dOriginalImageL = clCreateImage(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, \
    &format, &desc, OriginalImageL, &status);
    CHECK_CL_ERROR(status, "clCreateImage");
    
    cl_mem dOriginalImageR = clCreateImage(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, \
    &format, &desc, OriginalImageR, &status);
    CHECK_CL_ERROR(status, "clCreateImage");
    
    

    // Resizing kernel call
    SET_6_KERNEL_ARGS(resize_knl, dOriginalImageL, dOriginalImageR, dImageL, dImageR, Width, Height);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, resize_knl,
             2, NULL, (const size_t*)&globalSize, (const size_t*)&wgSize,
             0, NULL, NULL));
    CALL_CL_GUARDED(clFinish, (queue));
    
    CALL_CL_GUARDED(clEnqueueReadBuffer, (
    queue, dImageL, CL_TRUE,  0,
    Width*Height, Disparity,
    0, NULL, NULL));
      
    // Disparity LR zncc kernel call
    SET_11_KERNEL_ARGS(zncc_knl, dImageL, dImageR, dDisparityLR, Width, Height, BSX, BSY, MINDISP, MAXDISP, BSIZE, imsize);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, zncc_knl,
             2, NULL, (const size_t*)&globalSize, (const size_t*)&wgSize,
             0, NULL, NULL));

    // Disparity RL zncc kernel call
    tmp = MINDISP;
    MINDISP = -MAXDISP;
    MAXDISP = tmp;
    SET_11_KERNEL_ARGS(zncc_knl, dImageR, dImageL, dDisparityRL, Width, Height, BSX, BSY, MINDISP, MAXDISP, BSIZE, imsize);
    
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, zncc_knl,
             2, NULL, (const size_t*)&globalSize, (const size_t*)&wgSize,
             0, NULL, NULL));
    // Cross checking
    SET_5_KERNEL_ARGS(cross_check_knl, dDisparityLR, dDisparityRL, dDisparityLRCC, imsize, THRESHOLD);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, cross_check_knl,
             1, NULL, (const size_t*)&globalSize1D, (const size_t*)&wgSize1D,
             0, NULL, NULL));
    // occlusion filling
    SET_6_KERNEL_ARGS(occlusion_knl, dDisparityLRCC, dDisparity, Width, Height, NEIBSIZE, imsize);
    CALL_CL_GUARDED(clEnqueueNDRangeKernel,
            (queue, occlusion_knl,
             2, NULL, (const size_t*)&globalSize, (const size_t*)&wgSize,
             0, NULL, NULL));
    
    CALL_CL_GUARDED(clFinish, (queue));
    
    CALL_CL_GUARDED(clEnqueueReadBuffer, (
        queue, dDisparity, CL_TRUE,  0,
        Width*Height, Disparity,
        0, NULL, NULL));
        
    normalize_dmap(Disparity, Width, Height);
    
    clock_gettime(CLOCK_MONOTONIC, &finish);

    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    printf("Grid size: %d %d. Work Group size: %d %d, Elapsed time: %.4lf s.\n", (int) globalSize[0], (int) globalSize[1], (int) wgSize[0], (int) wgSize[1], elapsed);
    

    Error = lodepng_encode_file("depthmap.png", Disparity, Width, Height, LCT_GREY, 8);
    if(Error){
        printf("Error in saving of the disparity %u: %s\n", Error, lodepng_error_text(Error));
        return -1;
    }   

    
    FREE_ALL(OriginalImageR, OriginalImageL, Disparity, \
        resize_knl_text, zncc_knl_text, cross_check_knl_text, occlusion_knl_text);

    CALL_CL_GUARDED(clReleaseKernel, (resize_knl));
    CALL_CL_GUARDED(clReleaseKernel, (zncc_knl));
    CALL_CL_GUARDED(clReleaseKernel, (cross_check_knl));
    CALL_CL_GUARDED(clReleaseKernel, (occlusion_knl));

    CALL_CL_GUARDED(clReleaseCommandQueue, (queue));
    CALL_CL_GUARDED(clReleaseContext, (ctx));

    CALL_CL_GUARDED(clReleaseMemObject, (dOriginalImageL));
    CALL_CL_GUARDED(clReleaseMemObject, (dOriginalImageR));
    CALL_CL_GUARDED(clReleaseMemObject, (dImageL));
    CALL_CL_GUARDED(clReleaseMemObject, (dImageR));
    CALL_CL_GUARDED(clReleaseMemObject, (dDisparityLR));
    CALL_CL_GUARDED(clReleaseMemObject, (dDisparityRL));
    CALL_CL_GUARDED(clReleaseMemObject, (dDisparityLRCC));
    CALL_CL_GUARDED(clReleaseMemObject, (dDisparity));

	return 0;
}
