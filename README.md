# ZNCC 
Implementations in pure C and with OpenCL acceleration.

Tested environment configurations:
* GCC 4.8.4 64bit, OpenCL 1.2 with CUDA 7.5.

## How to run
You can choose the following options:

* Compile and run pure C implementation: `make c runc`
* Compile and run OpenCL implementation on GPU: `make cl runclgpu`
* Compile and run OpenCL implementation on CPU: `make cl runclcpu`

You can also just run already compiled version by removing first argument of teh commands given above.

