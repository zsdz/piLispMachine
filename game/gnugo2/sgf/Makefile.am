noinst_PROGRAMS = sgfgen

sgfgen_SOURCES = sgfgen.c

EXTRA_DIST = sgf_extras.def sgf_properties.def sgfgen.dsp sgf.dsp

noinst_HEADERS = \
	sgf.h sgf_utils.h ttsgf.h\
	ttsgf_read.h ttsgf_write.h\
	sgfana.h sgf_properties.h

GGBUILTSOURCES = sgf_properties.h

# Remove these files here... they are created locally
# don'e clean up sgfproperties.h
DISTCLEANFILES = *~

INCLUDES = -I$(top_srcdir)/engine -I$(top_srcdir)/interface

noinst_LIBRARIES = libsgf.a

libsgf_a_SOURCES = \
	sgf.c \
	sgf_utils.c \
	ttsgf.c \
	ttsgf_read.c \
	ttsgf_write.c \
        sgfana.c

sgf_properties.h: $(srcdir)/sgf_properties.def $(srcdir)/sgf_extras.def sgfgen$(EXEEXT)
	cat $(srcdir)/sgf_properties.def $(srcdir)/sgf_extras.def |\
		./sgfgen > sgf_properties.h
