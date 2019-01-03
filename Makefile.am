SUBDIRS = mdcomp src

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

mdtoolsdocdir = ${prefix}/doc/mdtools

mdtoolsdoc_DATA = \
	COPYING \
	INSTALL

EXTRA_DIST = $(mdtoolsdoc_DATA) \
	$(INTLTOOL_FILES)

DISTCLEANFILES =

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(mdtoolsdocdir)

