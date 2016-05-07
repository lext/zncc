all:
	gcc -c lodepng.c -O3
	gcc c_imp.c -o zncc lodepng.o -lm -lOpenCL -Wall -O3
	gcc c_ocl_imp.c -o zncc_ocl lodepng.o -lm -lOpenCL -Wall -O3
c:
	gcc c_imp.c -o zncc lodepng.o -lm -lOpenCL -Wall -O3
runc:
	./zncc im0.png im1.png
cl:
	gcc c_ocl_imp.c -o zncc_ocl lodepng.o -lm -lOpenCL -Wall -O3
runclgpu:
	./zncc_ocl im0.png im1.png GPU
runclcpu:
	./zncc_ocl im0.png im1.png CPU
clean:
	rm zncc zncc_ocl lodepng.o resized_left.png resized_right.png depthmap.png depthmap_no_post_procLR.png depthmap_no_post_procRL.png
	

