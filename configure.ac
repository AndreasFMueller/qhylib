#
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_REVISION([1.0])
AC_INIT([qhylib], [1.0])
LT_INIT
AC_CONFIG_SRCDIR([include/qhylib.h])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(include/config.h)

# some configuration directives

# Checks for programs
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Check for libraries
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([usb-1.0], [libusb_bulk_transfer])

AC_CHECK_HEADERS([unistd.h sys/time.h syslog.h])

AC_CONFIG_FILES([Makefile include/Makefile lib/Makefile
        src/Makefile])
AC_OUTPUT
