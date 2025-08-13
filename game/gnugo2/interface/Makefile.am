bin_PROGRAMS = gnugo

EXTRA_DIST = html interface.dsp gnugo.el

noinst_HEADERS = interface.h gmp.h 

# Remove these files here... they are created locally
DISTCLEANFILES = *~

INCLUDES = \
	-I$(top_srcdir)/sgf \
	-I$(top_srcdir)/engine \
	-I$(top_srcdir)/utils


LDADD = \
	../engine/libengine.a \
	../patterns/libpatterns.a \
	../sgf/libsgf.a \
	../utils/libutils.a

gnugo_SOURCES = \
	main.c \
	interface.c \
	play_ascii.c \
	play_gmp.c \
	play_solo.c \
	play_test.c \
	gmp.c

AIXOPTS=-O -qmaxmem=16384 -qro -qroconst -qinfo

# OPTIONS=$(AIXOPTS)
