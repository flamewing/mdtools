
bin_PROGRAMS = \
	plane_map \
	mapping_tool \
	recolor_art \
	smps2asm \
	voice_dumper \
	chunk_census \
	split_art \
	enitool \
	chunk_splitter \
	ssdraw

voice_dumper_SOURCES = \
	fmvoice.cc    \
	fmvoice.hh    \
	voice_dumper.cc

voice_dumper_CXXFLAGS = \
	-I$(srcdir)/../lib

chunk_census_SOURCES =  \
	chunk_census.cc

chunk_census_CXXFLAGS = -I$(srcdir)/../lib 

chunk_census_LDFLAGS = -lkosinski

split_art_SOURCES =  \
	split_art.cc

split_art_CXXFLAGS = -I$(srcdir)/../lib 

split_art_LDFLAGS = -lcomper -lkosinski \
	$(builddir)/../lib/libmappings.la

chunk_splitter_SOURCES =  \
	chunk_splitter.cc

chunk_splitter_CXXFLAGS = -I$(srcdir)/../lib 

ssdraw_SOURCES =  \
	ssexpand.cc

ssdraw_CXXFLAGS = -I$(srcdir)/../lib 

ssdraw_LDFLAGS = $(builddir)/../lib/libsstrack.la \
	-lenigma -lkosinski

smps2asm_SOURCES = \
	smps2asm.cc\
	fmvoice.cc\
	fmvoice.hh \
	songtrack.cc \
	songtrack.hh \
	../lib/ignore_unused_variable_warning.hh

smps2asm_CXXFLAGS = \
	-I$(srcdir)/../lib

smps2asm_LDFLAGS = -lsaxman

recolor_art_SOURCES = \
	recolor_art.cc

recolor_art_LDFLAGS = -lnemesis -lkosinski

recolor_art_CXXFLAGS = \
	-I$(srcdir)/../lib

mapping_tool_SOURCES = \
	mapping_tool.cc

mapping_tool_LDFLAGS = \
	$(builddir)/../lib/libmappings.la

mapping_tool_CXXFLAGS = \
	-I$(srcdir)/../lib

plane_map_SOURCES = \
	plane_map.cc \
	../lib/ignore_unused_variable_warning.hh

plane_map_LDFLAGS = -lenigma

plane_map_CXXFLAGS = \
	-I$(srcdir)/../lib

enitool_SOURCES = \
	enitool.cc

enitool_LDFLAGS = -lenigma

enitool_CXXFLAGS = \
	-I$(srcdir)/../lib

AM_CXXFLAGS =

## File created by the gnome-build tools

