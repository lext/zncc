#pragma OPENCL EXTENSION cl_khr_fp64: enable

__kernel void resize(__global uchar *imageL, __global  uchar *imageR, __global uchar *resizedL, __global  uchar *resizedR, int w, int h) {
	int i = get_global_id(0);
	int j = get_global_id(1);


}
