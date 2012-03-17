
bin_PROGRAMS = \
	plane_map \
	mapping_tool \
	recolor_art \
	smps2asm \
	voice_dumper

voice_dumper_SOURCES = \
	fmvoice.cc    \
	fmvoice.h    \
	voice_dumper.cc

voice_dumper_CXXFLAGS = \
	-I$(srcdir)/../lib

smps2asm_SOURCES = \
	smps2asm.cc\
	fmvoice.cc\
	fmvoice.h \
	songtrack.cc \
	songtrack.h

smps2asm_CXXFLAGS = \
	-I$(srcdir)/../lib

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
	plane_map.cc

plane_map_LDFLAGS = -lenigma

plane_map_CXXFLAGS = \
	-I$(srcdir)/../lib

AM_CXXFLAGS =

## File created by the gnome-build tools

