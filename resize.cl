__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE| CLK_ADDRESS_CLAMP_TO_EDGE| CLK_FILTER_NEAREST;

__kernel void resize(__read_only image2d_t imageL, __read_only image2d_t imageR, __global uchar *resizedL, __global  uchar *resizedR, int new_w, int new_h) {
	const int i = get_global_id(0);
	const int j = get_global_id(1);
	
    // Calculating corresponding indices in the original image
    int2 orig_pos = {(4*j-1*(j > 0)), (4*i-1*(i > 0))};
    // Grayscaling and resizing
    uint4 pxL = read_imageui(imageL, sampler, orig_pos);
    uint4 pxR = read_imageui(imageR, sampler, orig_pos);
    
    resizedL[i*new_w+j] = 0.2126*pxL.x+0.7152*pxL.y+0.0722*pxL.z;
    resizedR[i*new_w+j] = 0.2126*pxR.x+0.7152*pxR.y+0.0722*pxR.z;

}
