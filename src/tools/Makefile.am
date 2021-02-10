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

voice_dumper_SOURCES = fmvoice.cc voice_dumper.cc

chunk_census_SOURCES = chunk_census.cc
chunk_census_LDADD = -lkosinski

split_art_SOURCES = split_art.cc
split_art_LDADD = -lcomper -lkosinski \
	$(abs_top_builddir)/src/lib/libmappings.la

chunk_splitter_SOURCES = chunk_splitter.cc

ssdraw_SOURCES = ssexpand.cc
ssdraw_LDADD = $(abs_top_builddir)/src/lib/libsstrack.la \
	-lenigma -lkosinski

smps2asm_SOURCES = smps2asm.cc\
	fmvoice.cc\
	songtrack.cc
smps2asm_LDADD = -lsaxman

recolor_art_SOURCES = recolor_art.cc
recolor_art_LDADD = -lnemesis -lkosinski

mapping_tool_SOURCES = mapping_tool.cc
mapping_tool_LDADD = $(abs_top_builddir)/src/lib/libmappings.la

plane_map_SOURCES = plane_map.cc
plane_map_LDADD = -lenigma

enitool_SOURCES = enitool.cc
enitool_LDADD = -lenigma

AM_LDFLAGS = -L$(abs_top_srcdir)/mdcomp/src/lib/.libs
AM_CPPFLAGS = -I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/mdcomp/include -I$(abs_top_srcdir)/fmt/include
