AC_PREREQ([2.69])
AC_INIT([httee], [0.1], [romain@blogreen.org])
AC_CONFIG_SRCDIR([httee.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
PKG_CHECK_MODULES([LIBXDG_BASEDIR], [libxdg-basedir])

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([mkdir realpath strdup strstr])

CFLAGS="$CFLAGS -std=c99"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
