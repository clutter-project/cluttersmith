# autoconf
AC_PREREQ(2.53)
AC_INIT([cluttersmith],[0.0],[Clutter Scene Graph Editor])
AC_CONFIG_MACRO_DIR([autotools])
AC_CONFIG_SRCDIR(src/cluttersmith.h)
AC_CONFIG_HEADERS([config.h])

# automake
AM_INIT_AUTOMAKE([1.10 foreign -Wno-portability no-define])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AS_AM_REALLY_SILENT
AM_MAINTAINER_MODE

# libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Check for programs
AC_PROG_CC
PKG_PROG_PKG_CONFIG([0.16])
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

# Check for headers
AC_HEADER_STDC

# Check for dependencies
PKG_CHECK_MODULES(DEPS, [clutter-1.0 \
                         clutter-gst-1.0 \
                         glib-2.0 >= 2.22.4 \
                         mx-1.0 \
                         gjs-1.0])
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AS_COMPILER_FLAGS([COMPILER_FLAGS], ["-Wall"])
AC_SUBST(COMPILER_FLAGS)

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

AC_CONFIG_FILES([
Makefile
autotools/Makefile
src/Makefile
data/Makefile
data/templates/Makefile
data/annotation-templates/Makefile
data/docs/Makefile
])
AC_OUTPUT
