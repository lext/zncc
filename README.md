# ZNCC 
Implementations in pure C and with OpenCL acceleration.

Tested environment configurations:
* GCC 4.8.4 64bit, OpenCL 1.2 with CUDA 7.5.

## How to run
You can choose the following options:

* Compile and run pure C implementation: `make c runc`
* Compile and run OpenCL implementation: `make cl runcl`

You can also just run already compiled version by removing first argument of the commands given above. If you want to compile all, just type `make all` and the you can use `make runc` etc.

In case if you want to play with the parameters of the programs, their description is given below.

## Parameters

* C verison (zncc) takes two parameters: `Left_Image Right_Image`
* OpenCL C version (zncc_ocl) takes six parameters: `Left_Image Right_Image wg_size0 wg_size1 global_size0 global_size1`.

You can look at the content of the Makefile to see how the programs are compiled and called.
