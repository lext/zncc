all:
	gcc -c lodepng.c -O3
	gcc c_imp.c -o zncc lodepng.o -lm -Wall -O3
	./zncc im0.png im1.png
run:
	./zncc im0.png im1.png
clean:
	rm zncc lodepng.o resized_left.png resized_right.png depthmap.png depthmap_no_post_procLR.png depthmap_no_post_procRL.png
