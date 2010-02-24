AC_PREREQ(2.53)
AC_INIT(cluttersmith, 0.0, [Clutter Scene Graph Editor])
AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([autotools])
AC_CONFIG_SRCDIR(src/cluttersmith.h)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
AC_STDC_HEADERS

PKG_PROG_PKG_CONFIG(0.16)

PKG_CHECK_MODULES(DEPS, clutter-1.0 \
                        clutter-gst-1.0 \
                        glib-2.0 >= 2.22.0 \
                        mx-1.0 \
                        gjs-1.0 )
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

if test "x$GCC" = "xyes"; then
        GCC_FLAGS="-g -Wall"
fi

AC_SUBST(GCC_FLAGS)

# Sets GLIB_GENMARSHAL and GLIB_MKENUMS
AM_PATH_GLIB_2_0()
G_IR_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
AC_SUBST(G_IR_SCANNER)
G_IR_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`
AC_SUBST(G_IR_COMPILER)
G_IR_GENERATE=`$PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0`
AC_SUBST(G_IR_GENERATE)
GIRDIR=`$PKG_CONFIG --variable=girdir gobject-introspection-1.0`
AC_SUBST(GIRDIR)
TYPELIBDIR="$($PKG_CONFIG --variable=typelibdir gobject-introspection-1.0)"
AC_SUBST(TYPELIBDIR)


AC_OUTPUT([
Makefile
autotools/Makefile
src/Makefile
data/Makefile
data/templates/Makefile
data/annotation-templates/Makefile
data/docs/Makefile
])
