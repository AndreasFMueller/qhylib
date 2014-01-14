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

test:	qhycooler
	./qhycooler -d
