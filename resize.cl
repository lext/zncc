#pragma OPENCL EXTENSION cl_khr_fp64: enable

__kernel void resize(__global uchar *imageL, __global  uchar *imageR, __global uchar *resizedL, __global  uchar *resizedR, int w, int h) {
	int i = get_global_id(0);
	int j = get_global_id(1);
    
    if ((i < h) && (j < w)) {

        int new_w=w/4, new_h=h/4; //  Width and height of the downscaled image
        int orig_i, orig_j; // Indices of the original image

        // Calculating corresponding indices in the original image
        orig_i = (4*i-1*(i > 0)); 
        orig_j = (4*j-1*(j > 0));
        // Grayscaling and resizing
        resizedL[i*new_w+j] = 0.2126*imageL[orig_i*(4*w)+4*orig_j]+0.7152*imageL[orig_i*(4*w)+4*orig_j + 1]+0.0722*imageL[orig_i*(4*w)+4*orig_j + 2];
        resizedR[i*new_w+j] = 0.2126*imageR[orig_i*(4*w)+4*orig_j]+0.7152*imageR[orig_i*(4*w)+4*orig_j + 1]+0.0722*imageR[orig_i*(4*w)+4*orig_j + 2];
	}


}
