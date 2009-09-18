QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

bin_PROGRAMS=cluttersmith

AM_CFLAGS = $(DEPS_CFLAGS) $(GCC_FLAGS) \
	    -D_GNU_SOURCE \
	    -DPKGDATADIR=\"$(pkgdatadir)/\" \
	    -DLOCALEDIR=\""$(localedir)"\"

BUILT_SOURCES = $(cluttersmith_built_sources)
CLEANFILES    = $(cluttersmith_built_sources)

cluttersmith_LDADD  = $(DEPS_LIBS)
cluttersmith_LDFLAGS = -rdynamic
cluttersmith_SOURCES = 		\
  $(BUILT_SOURCES) \
  actor-editing.c  \
  cb-texture.c  \
  cb-texture.h  \
  cluttersmith.c  \
  cluttersmith.h  \
  hyperlink.c  \
  init-types.c  \
  json-serializer.c  \
  main.c  \
  popup.c  \
  popup.h  \
  previews.c  \
  property-editors.c  \
  scene-graph.c  \
  selection.c  \
  session-history.c  \
  stencils.c  \
  util.c  \
  util.h  \
  verbs.c


test: cluttersmith
	./cluttersmith

gdb: all
		gdb --args ./cluttersmith
gdb2: all
		gdb --args ./cluttersmith --g-fatal-warnings
valgrind: all
		valgrind --leak-check=full ./cluttersmith


# the following are local small hacks
CLUTTER_HEADER_DIR=/usr/local/include/clutter-1.0/clutter/
NBTK_HEADER_DIR=/usr/local/include/nbtk-1.0/nbtk/

init-types.c:  
	echo "#include <clutter/clutter.h>" > $@
	echo "#include <nbtk/nbtk.h>" >> $@
	echo "void init_types(void){gint i = 0;" >> $@
	for i in `grep CLUTTER_TYPE_ $(CLUTTER_HEADER_DIR)*.h | sed "s/.*CLUTTER_TYPE_/CLUTTER_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	for i in `grep NBTK_TYPE_ $(NBTK_HEADER_DIR)*.h | sed "s/.*NBTK_TYPE_/NBTK_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	echo "}" >> $@


cluttersmith.so: *.c Makefile
	gcc -DCOMPILEMODULE $(AM_CFLAGS) -g -shared -ldl *.c -o $@ `pkg-config clutter-1.0 clutter-gtk-0.10 nbtk-1.0 --libs --cflags` 




