#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([pings], [0.1.0], [dev@swift-nav.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/ntrip_ping.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([m], [cos])
LIBCURL_CHECK_CONFIG

# Checks for header files.
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_CHECK_HEADER([getopt.h])
AC_CHECK_HEADER([inttypes.h])
AC_CHECK_HEADER([math.h])
AC_CHECK_HEADER([stdbool.h])
AC_CHECK_HEADER([stdlib.h])
AC_CHECK_HEADER([string.h])
AC_CHECK_HEADER([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.


AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT
