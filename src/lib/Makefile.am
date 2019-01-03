lib_LTLIBRARIES = libmappings.la libsstrack.la

libmappings_la_SOURCES = \
	singledplc.cc \
	framedplc.cc \
	dplcfile.cc \
	singlemapping.cc \
	framemapping.cc \
	mappingfile.cc

libsstrack_la_SOURCES = ssvram.cc sstrack.cc
libsstrack_la_LIBADD = -lkosinski -lenigma

AM_LDFLAGS = -L$(srcdir)/../../mdcomp/src/lib/.libs
AM_CPPFLAGS = -I$(srcdir)/../../include -I$(srcdir)/../../mdcomp/include

