
bin_PROGRAMS = \
	plane_map \
	mapping_tool \
	recolor_art

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

