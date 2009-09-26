AC_PREREQ(2.53)
AC_INIT(cluttersmith, 0.0, [Clutter Scene Graph Editor])
AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([autotools])
AC_CONFIG_SRCDIR(src/cluttersmith.h)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS

PKG_CHECK_MODULES(DEPS, clutter-1.0 \
                        clutter-gst-0.90 \
                        nbtk-1.2)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

if test "x$GCC" = "xyes"; then
        GCC_FLAGS="-g -Wall"
fi

AC_SUBST(GCC_FLAGS)

SHAVE_INIT([autotools], [enable])

AC_OUTPUT([
Makefile
autotools/Makefile
autotools/shave
src/Makefile
data/Makefile
data/stencils/Makefile
data/docs/Makefile
])
