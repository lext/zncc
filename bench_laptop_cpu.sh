#! /bin/bash
echo "Running benchmarks with 1024 x 1024 grid and workgroups having size power of 2"
export OUT_DEVINFO=0
export CL_HELPER_NO_COMPILER_OUTPUT_NAG=1

./zncc img0.png im1.png 8 8 1024 1024
./zncc img0.png im1.png 8 8 1024 1024
./zncc img0.png im1.png 8 8 1024 1024

./zncc img0.png im1.png 16 16 1024 1024
./zncc img0.png im1.png 16 16 1024 1024
./zncc img0.png im1.png 16 16 1024 1024

./zncc img0.png im1.png 32 32 1024 1024
./zncc img0.png im1.png 32 32 1024 1024
./zncc img0.png im1.png 32 32 1024 1024

echo "Running benchmarks with Height x Width grid"
./zncc img0.png im1.png 12 15 504 735
./zncc img0.png im1.png 21 21 504 735
./zncc img0.png im1.png 21 21 504 735
./zncc img0.png im1.png 21 21 504 735
./zncc img0.png im1.png 21 21 504 735
./zncc img0.png im1.png 21 21 504 735
./zncc img0.png im1.png 2 3 504 735
./zncc img0.png im1.png 2 5 504 735
./zncc img0.png im1.png 2 7 504 735
./zncc img0.png im1.png 2 15 504 735
./zncc img0.png im1.png 2 21 504 735
./zncc img0.png im1.png 3 3 504 735
./zncc img0.png im1.png 3 5 504 735
./zncc img0.png im1.png 3 7 504 735
./zncc img0.png im1.png 3 15 504 735
./zncc img0.png im1.png 3 21 504 735
./zncc img0.png im1.png 4 3 504 735
./zncc img0.png im1.png 4 5 504 735
./zncc img0.png im1.png 4 7 504 735
./zncc img0.png im1.png 12 3 504 735
./zncc img0.png im1.png 12 5 504 735
./zncc img0.png im1.png 12 7 504 735
./zncc img0.png im1.png 12 15 504 735
./zncc img0.png im1.png 12 21 504 735
./zncc img0.png im1.png 14 3 504 735
./zncc img0.png im1.png 14 5 504 735
./zncc img0.png im1.png 14 7 504 735
./zncc img0.png im1.png 14 15 504 735
./zncc img0.png im1.png 14 21 504 735
./zncc img0.png im1.png 18 3 504 735
./zncc img0.png im1.png 18 5 504 735
./zncc img0.png im1.png 18 7 504 735
./zncc img0.png im1.png 18 15 504 735
./zncc img0.png im1.png 18 21 504 735
./zncc img0.png im1.png 21 3 504 735
./zncc img0.png im1.png 21 5 504 735
./zncc img0.png im1.png 21 7 504 735
./zncc img0.png im1.png 21 15 504 735
./zncc img0.png im1.png 21 21 504 735

export OUT_DEVINFO=1
export CL_HELPER_NO_COMPILER_OUTPUT_NAG=0





