SUBDIRS = vc71

bin_PROGRAMS = modeltool

modeltool_SOURCES =	\
	model.h		\
	modeltool.cxx	\
	Q3BSP.cxx	\
	Q3BSP.h		\
	wavefrontOBJ.cxx\
	wavefrontOBJ.h

modeltool_CPPFLAGS = \
	-I$(top_srcdir)/include

MAINTAINERCLEANFILES = \
	Makefile.in

LDADD = \
	$(top_srcdir)/src/common/libCommon.la

