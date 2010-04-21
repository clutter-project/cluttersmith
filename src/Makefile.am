bin_PROGRAMS=cluttersmith
lib_LTLIBRARIES = libcluttersmith.la

AM_CFLAGS = $(DEPS_CFLAGS) $(COMPILER_FLAGS) \
	    -D_GNU_SOURCE \
	    -DPKGDATADIR=\"$(pkgdatadir)/\" \
	    -DPKGLIBDIR=\"$(pkglibdir)/\" \
	    -DLOCALEDIR=\""$(localedir)"\"

BUILT_SOURCES = $(cluttersmith_built_sources)
CLEANFILES    = $(cluttersmith_built_sources)

  
libcluttersmith_la_SOURCES = \
cluttersmith.h \
clutter-states.c \
clutter-states.h \
cs-event-dispatch.c \
cs-animator-editor.c \
cs-animator-editor.h \
cs-bind.c \
cs-callbacks.c \
cs-context.c \
cs-context.h \
cs-export.c \
cs-history.c \
cs-history.h \
cs-selection.c \
cs-serializer.c \
cs-session-history.c \
cs-stencils.c \
cs-resize.c \
cs-move.c \
cs-edit-text.c \
cs-texture.c \
cs-texture.h \
cs-tree.c \
cs-util.c \
cs-util.h \
cs-pan.c \
cs-verbs.c \
init-types.c \
property-editors.c


libcluttersmith_la_LIBADD = $(DEPS_LIBS)
libcluttersmith_la_LDFLAGS = -rdynamic

cluttersmith_LDADD  = libcluttersmith.la $(DEPS_LIBS)
cluttersmith_LDFLAGS = -rdynamic
cluttersmith_SOURCES = 		\
  $(BUILT_SOURCES) \
  cluttersmith.c 

CLEANFILES += libcluttersmith.la

girdir = $(datadir)/gir-1.0
gir_DATA = ClutterSmith-0.0.gir

ClutterSmith-0.0.gir: $(INTROSPECTION_SCANNER) libcluttersmith.la Makefile $(libcluttersmith_la_SOURCES)
	$(AM_V_GEN) $(INTROSPECTION_SCANNER)			 \
		--namespace=ClutterSmith		 \
		--nsversion=0.1				 \
		--include=Clutter-1.0			 \
		--include=Mx-1.0			 \
		--libtool="$(LIBTOOL)"    		 \
		--add-include-path=$(builddir)     	 \
		--library=libcluttersmith.la 		 \
		$(srcdir)/*.c $(srcdir)/*.h 		 \
		$(libcluttersmith_la_CPPFLAGS)		 \
		`pkg-config --cflags mx-1.0 clutter-1.0 ` \
		-o $@
CLEANFILES += ClutterSmith-0.0.gir

typelibdir = $(libdir)/girepository-1.0/
typelib_DATA = ClutterSmith-0.0.typelib

# The dependency on libcluttersmith.la here is because g-ir-compiler opens it
# (not the fake library, since we've already done the rewriting)
ClutterSmith-0.0.typelib: ClutterSmith-0.0.gir libcluttersmith.la
	$(AM_V_GEN) $(INTROSPECTION_COMPILER)		\
		--includedir=.				\
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


