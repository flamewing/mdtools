lib_LTLIBRARIES = libmappings.la libsstrack.la

noinst_LIBRARIES = libstaticanalyzer.a

libmappings_la_SOURCES = \
	singledplc.cc \
	framedplc.cc \
	dplcfile.cc \
	singlemapping.cc \
	framemapping.cc \
	mappingfile.cc

libsstrack_la_SOURCES = ssvram.cc sstrack.cc
libsstrack_la_LIBADD = -lkosinski -lenigma

libstaticanalyzer_a_SOURCES = \
	fmvoice.cc \
	ignore_unused_variable_warning.cc \
	pattern_name.cc \
	pattern_name_table.cc \
	songtrack.cc \
	tile.cc \
	vram.cc

AM_LDFLAGS = -no-undefined -L$(srcdir)/../../mdcomp/src/lib/.libs
AM_CPPFLAGS = -I$(srcdir)/../../include -I$(srcdir)/../../mdcomp/include -I$(srcdir)/../../fmt/include

