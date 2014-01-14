#
# Makefile.am -- build the client programs
#
# (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
#
noinst_PROGRAMS = qhycooler qhycamera

qhycamera_SOURCES = qhycamera.cpp
qhycamera_DEPENDENCIES = ../lib/libqhyccd.la
qhycamera_LDADD = -L../lib -lqhyccd -lcfitsio

qhycooler_SOURCES = qhycooler.cpp
qhycooler_DEPENDENCIES = ../lib/libqhyccd.la
qhycooler_LDADD = -L../lib -lqhyccd

test:	qhycamera
	./qhycamera -d -b 4 -e 0.05 -p 0x6003 test.fits

coolertest:	qhycooler
	./qhycooler -d
