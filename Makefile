clutterbug.so: *.c Makefile
	gcc -g -shared -ldl *.c -o clutterbug.so `pkg-config clutter-1.0 clutter-gtk-0.10 nbtk-1.0 --libs --cflags` 

clean:
	rm -f *.so
