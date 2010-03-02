bin_PROGRAMS=cluttersmith
lib_LTLIBRARIES = libcluttersmith.la

AM_CFLAGS = $(DEPS_CFLAGS) $(GCC_FLAGS) \
	    -D_GNU_SOURCE \
	    -DPKGDATADIR=\"$(pkgdatadir)/\" \
	    -DPKGLIBDIR=\"$(pkglibdir)/\" \
	    -DLOCALEDIR=\""$(localedir)"\"

BUILT_SOURCES = $(cluttersmith_built_sources)
CLEANFILES    = $(cluttersmith_built_sources)

  
libcluttersmith_la_SOURCES = \
  cb-texture.c  \
  cb-texture.h  \
  actor-editing.c  \
  animator-editor.c \
  animator-editor.h \
  cs-context.c  \
  cs-context.h  \
  cluttersmith.h  \
  hyperlink.c  \
  init-types.c  \
  json-serializer.c  \
  popup.c  \
  popup.h  \
  previews.c  \
  property-editors.c  \
  scene-graph.c  \
  selection.c  \
  session-history.c  \
  verbs.c \
  stencils.c  \
  util.c  \
  util.h  \
  clutter-states.c \
  clutter-states.h

libcluttersmith_la_LIBADD = $(DEPS_LIBS)
libcluttersmith_la_LDFLAGS = -rdynamic

cluttersmith_LDADD  = libcluttersmith.la $(DEPS_LIBS)
cluttersmith_LDFLAGS = -rdynamic
cluttersmith_SOURCES = 		\
  $(BUILT_SOURCES) \
  main.c 

typelibdir = $(libdir)/girepository-1.0/
typelib_DATA = ClutterSmith-0.0.typelib
CLEANFILES += libcluttersmith.la

ClutterSmith-0.0.gir: $(G_IR_SCANNER) libcluttersmith.la Makefile $(libcluttersmith_la_SOURCES)
	$(AM_V_GEN) $(G_IR_SCANNER)			 \
		--namespace=ClutterSmith		 \
		--nsversion=0.1				 \
		--include=Clutter-1.0			 \
		--libtool="$(LIBTOOL)"    		 \
		--add-include-path=$(builddir)     	 \
		--library=libcluttersmith.la 		 \
		$(srcdir)/*.c $(srcdir)/*.h 		 \
		$(libcluttersmith_la_CPPFLAGS)		 \
		`pkg-config --cflags clutter-1.0 ` \
		-o $@
CLEANFILES += ClutterSmith-0.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = ClutterSmith-0.0.gir


# The dependency on libgnome-shell.la here is because g-ir-compiler opens it
# (not the fake library, since we've already done the rewriting)
ClutterSmith-0.0.typelib: ClutterSmith-0.0.gir libcluttersmith.la
	$(AM_V_GEN) \
		$(G_IR_COMPILER)						\
			--includedir=.						\
		ClutterSmith-0.0.gir -o $@
CLEANFILES += ClutterSmith-0.0.typelib




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
MX_HEADER_DIR=/usr/local/include/mx-1.0/mx/

init-types.c:  
	echo "#include <clutter/clutter.h>" > $@
	echo "#include <mx/mx.h>" >> $@
	echo "void init_types(void){gint i = 0;" >> $@
	for i in `grep CLUTTER_TYPE_ $(CLUTTER_HEADER_DIR)*.h | sed "s/.*CLUTTER_TYPE_/CLUTTER_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	    for i in `grep MX_TYPE_ $(MX_HEADER_DIR)*.h | sed "s/.*MX_TYPE_/MX_TYPE_/" | sed "s/[^_A-Z].*$$//"|sort|uniq`;do \
	    echo "i = $$i;" >> $@\
	 ;done
	echo "}" >> $@


cluttersmith.so: *.c Makefile
	gcc -DCOMPILEMODULE $(AM_CFLAGS) -I. -g -shared -ldl *.c -o $@ `pkg-config clutter-1.0 gjs-1.0 mx-1.0 clutter-gst-1.0 --libs --cflags` 


