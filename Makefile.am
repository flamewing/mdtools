## Process this file with automake to produce Makefile.in
## Created by Anjuta

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = src

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

