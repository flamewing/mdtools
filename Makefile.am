SUBDIRS = mdcomp src

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

mdtoolsdocdir = ${prefix}/doc/mdtools

mdtoolsdoc_DATA = \
	COPYING \
	INSTALL

EXTRA_DIST = $(mdtoolsdoc_DATA)

DISTCLEANFILES =

includedir = ${prefix}/include/mdtools

include_HEADERS = \
	include/mdtools/dplcfile.hh \
	include/mdtools/fmvoice.hh \
	include/mdtools/framedplc.hh \
	include/mdtools/framemapping.hh \
	include/mdtools/ignore_unused_variable_warning.hh \
	include/mdtools/mappingfile.hh \
	include/mdtools/pattern_name.hh \
	include/mdtools/pattern_name_table.hh \
	include/mdtools/singledplc.hh \
	include/mdtools/singlemapping.hh \
	include/mdtools/songtrack.hh \
	include/mdtools/sstrack.hh \
	include/mdtools/ssvram.hh \
	include/mdtools/tile.hh \
	include/mdtools/vram.hh

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(mdtoolsdocdir)
