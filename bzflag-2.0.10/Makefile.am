# $Id: Makefile.am 16029 2007-10-08 18:42:37Z macsforme $

AUTOMAKE_OPTIONS = 1.6 dist-zip dist-bzip2
ACLOCAL_AMFLAGS = -I m4

traversal_dirs = \
	data \
	doc \
	include \
	man \
	package \
	src

if BUILD_PLUGINS
PLUGINS=plugins
else
PLUGINS=
endif

SUBDIRS = \
	$(traversal_dirs) \
	$(PLUGINS)

DIST_SUBDIRS = \
	$(traversal_dirs) \
	Dev-C++ \
	debian \
	m4 \
	misc \
	plugins \
	tools \
	win32 \
	BZFlag.xcodeproj

EXTRA_DIST = \
	autogen.sh \
	authors.xml \
	BUGS \
	bzflag.info \
	bzflag.lsm \
	bzflag.spec \
	DEVINFO \
	PORTING \
	README.BeOS \
	README.DEVC++ \
	README.IRIX \
	README.Linux \
	README.MINGW32 \
	README.MacOSX \
	README.SDL \
	README.SOLARIS \
	README.UNIX \
	README.WIN32 \
	README.XMINGW	\
	RELNOTES

MAINTAINERCLEANFILES = \
	configure \
	bzflag.info \
	bzflag.spec \
	INSTALL \
	Makefile.in

.PHONY: package debian-svn debian-release

package:
	cd package && $(MAKE) targets

debian-svn: dist
	-rm -rf $(distdir)
	GZIP=$(GZIP_ENV) $(AMTAR)$(TAR) zxf $(distdir).tar.gz
	(cd $(distdir) && debian/buildsnap svn)

debian-release: debian/changelog dist
	-rm -rf $(distdir)
	GZIP=$(GZIP_ENV) $(AMTAR)$(TAR) zxf $(distdir).tar.gz
	cp debian/changelog $(distdir)/debian/
	(cd $(distdir) && debian/buildsnap release)

