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

# if cfits is installed with pkginfo, we use that
if pkg-config --exists cfitsio
then
        LIBS="${LIBS} `pkg-config --libs cfitsio`"
        CXXFLAGS="${CXXFLAGS} `pkg-config --cflags cfitsio`"
else
AC_CHECK_LIB([cfitsio], [ftffrw_])
fi


# probe for some special functions that not all USB library versions have
AC_CHECK_FUNC([libusb_strerror],[
AC_DEFINE([HAVE_LIBUSB_STRERROR], 1, [libusb has libusb_strerror function])
],[
AC_DEFINE([HAVE_LIBUSB_STRERROR], 0, [libusb has libusb_strerror function])
])

AC_CHECK_FUNC([libusb_set_debug],[
AC_DEFINE([HAVE_LIBUSB_SET_DEBUG], 1, [libusb has libusb_set_debug function])
],[
AC_DEFINE([HAVE_LIBUSB_SET_DEBUG], 0, [libusb has libusb_set_debug function])
])

# enable usb debugging
AC_ARG_ENABLE(usbdebug,
[AS_HELP_STRING([--enable-usbdebug], [turn on USB low level debugging])],
[
AC_DEFINE([USBDEBUG], 1, [Whether or not to enable USB debugging])
],[
AC_DEFINE([USBDEBUG], 0, [Whether or not to enable USB debugging])
])

# enable range checking
AC_ARG_ENABLE(rangecheck,
[AS_HELP_STRING([--enable-rangecheck], [turn on range checking])],
[
AC_DEFINE([ENABLE_RANGECHECK], 1, [Whether or not to enable range checking])
],[
AC_DEFINE([ENABLE_RANGECHECK], 0, [Whether or not to enable range checking])
])

AC_CHECK_HEADERS([unistd.h sys/time.h syslog.h signal.h pthread.h math.h])

AC_CONFIG_FILES([Makefile include/Makefile lib/Makefile src/Makefile
	doc/Makefile])

AC_OUTPUT
