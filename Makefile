all:
	gcc -c lodepng.c
	gcc c_imp.c -o zncc lodepng.o -Wall
	./zncc im0.png im1.png
run:
	./zncc im0.png im1.png
clean:
	rm zncc lodepng.o resized_left.png resized_right.png
