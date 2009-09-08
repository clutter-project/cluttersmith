clutterbug.so: *.c Makefile
	gcc -g -shared -ldl *.c -o clutterbug.so `pkg-config clutter-1.0 clutter-gtk-0.10 nbtk-1.0 --libs --cflags` 

CLUTTER_HEADERS=/usr/local/include/clutter-1.0/clutter/
NBTK_HEADERS=/usr/local/include/nbtk-1.0/nbtk/

init-types.c:  Makefile
	echo "#include <clutter/clutter.h>" > $@
	echo "#include <nbtk/nbtk.h>" >> $@
	echo "void init_types(void){gint i = 0;" >> $@
	for i in `grep CLUTTER_TYPE_ $(CLUTTER_HEADERS)*.h | sed "s/.*CLUTTER_TYPE_/CLUTTER_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	for i in `grep NBTK_TYPE_ $(NBTK_HEADERS)*.h | sed "s/.*NBTK_TYPE_/NBTK_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	echo "}" >> $@

clean:
	rm -f *.so
