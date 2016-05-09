all:
	gcc -std=gnu99 -c lodepng.c -O3
	gcc -std=gnu99 -c cl-helper.c -O3
	gcc -std=gnu99 c_imp.c -o zncc lodepng.o -lm -Wall -O3
	gcc -std=gnu99 c_ocl_imp.c -o zncc_ocl lodepng.o -lm -lOpenCL -Wall -O3
c:
	gcc -std=gnu99 -c lodepng.c -O3
	gcc -std=gnu99 c_imp.c -o zncc lodepng.o -lm  -Wall -O3
runc:
	./zncc im0.png im1.png
cl:
	gcc -std=gnu99 -c lodepng.c -O3
	gcc -std=gnu99 -c cl-helper.c -O3
	gcc -std=gnu99 c_ocl_imp.c -o zncc_ocl lodepng.o cl-helper.o -lm -lOpenCL -Wall -O3
runcl:
	./zncc_ocl im0.png im1.png
clean:
	rm zncc zncc_ocl lodepng.o cl-helper.o resized_left.png resized_right.png depthmap.png depthmap_no_post_procLR.png depthmap_no_post_procRL.png
	

