#
# Makefile.am -- build the client programs
#
# (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
#
noinst_PROGRAMS = qhycooler qhycamera qhytransfer

qhycamera_SOURCES = qhycamera.cpp
qhycamera_DEPENDENCIES = ../lib/libqhyccd.la
qhycamera_LDADD = -L../lib -lqhyccd -lcfitsio

qhycooler_SOURCES = qhycooler.cpp
qhycooler_DEPENDENCIES = ../lib/libqhyccd.la
qhycooler_LDADD = -L../lib -lqhyccd

qhytransfer_SOURCES = qhytransfer.cpp
qhytransfer_DEPENDENCIES = ../lib/libqhyccd.la
qhytransfer_LDADD = -L../lib -lqhyccd

test:	qhycamera
	./qhycamera -d -e 1 -p 0x6003 test.fits

coolertest:	qhycooler
	./qhycooler -d

transfer:	qhytransfer
	./qhytransfer -d -n 30 -s 1 -m 10
