EXTRA_DIST = gnugo.dsp

# Remove these files here... they are created locally
DISTCLEANFILES = *~

# source files in this directory get access to private prototypes
INCLUDES = \
	-I../patterns \
	-I$(top_srcdir)/interface \
	-I$(top_srcdir)/patterns \
	-I$(top_srcdir)/sgf \
	-I$(top_srcdir)/utils -DBUILDING_GNUGO_ENGINE

noinst_HEADERS = hash.h liberty.h main.h

# preconfigured settings for various configurations
noinst_LIBRARIES = libengine.a

libengine_a_SOURCES = \
      attdef.c \
      dragon.c \
      filllib.c \
      fuseki.c \
      genmove.c \
      globals.c \
      hash.c \
      matchpat.c \
      moyo.c \
      optics.c \
      reading.c \
      semeai.c \
      sethand.c \
      shapes.c \
      showbord.c \
      utils.c \
      worm.c
