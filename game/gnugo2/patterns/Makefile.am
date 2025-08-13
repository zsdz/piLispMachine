noinst_PROGRAMS = mkpat joseki mkeyes

DSP = joseki.dsp mkeyes.dsp mkpat.dsp patterns.dsp

EXTRA_DIST = $(DSP)\
	fuseki2.sgf\
	hoshi.sgf\
	komoku.sgf\
	mokuhazushi.sgf\
	sansan.sgf\
	takamoku.sgf\
	patterns.db\
	eyes.db\
	conn.db

mkpat_SOURCES  = mkpat.c
mkpat_LDADD = ../utils/libutils.a
joseki_SOURCES = joseki.c
mkeyes_SOURCES = mkeyes.c

noinst_HEADERS = patterns.h eyes.h

GGBUILTSOURCES = conn.c patterns.c eyes.c

DBBUILT = hoshi.db komoku.db sansan.db mokuhazushi.db takamoku.db

# Remove these files here... they are created locally
DISTCLEANFILES = $(GGBUILTSOURCES) $(DBBUILT) *~

dist-hook:
	cd $(distdir) && rm $(GGBUILTSOURCES)

# source files in this directory get access to private prototypes
INCLUDES = -I$(top_srcdir)/engine -I$(top_srcdir)/utils -DBUILDING_GNUGO_ENGINE

noinst_LIBRARIES = libpatterns.a

libpatterns_a_SOURCES = connections.c helpers.c $(GGBUILTSOURCES)

hoshi.db : $(srcdir)/hoshi.sgf joseki$(EXEEXT)
	./joseki JH < $(srcdir)/hoshi.sgf >hoshi.db

komoku.db : $(srcdir)/komoku.sgf joseki$(EXEEXT)
	./joseki JK < $(srcdir)/komoku.sgf >komoku.db

sansan.db : $(srcdir)/sansan.sgf joseki$(EXEEXT)
	./joseki JS < $(srcdir)/sansan.sgf >sansan.db

mokuhazushi.db : $(srcdir)/mokuhazushi.sgf joseki$(EXEEXT)
	./joseki JM < $(srcdir)/mokuhazushi.sgf >mokuhazushi.db

takamoku.db : $(srcdir)/takamoku.sgf joseki$(EXEEXT)
	./joseki JT < $(srcdir)/takamoku.sgf >takamoku.db

patterns.c : $(srcdir)/patterns.db $(DBBUILT) mkpat$(EXEEXT)
	cat  $(srcdir)/patterns.db $(DBBUILT) | ./mkpat pat >patterns.c

conn.c : $(srcdir)/conn.db mkpat$(EXEEXT)
	./mkpat -c conn < $(srcdir)/conn.db >conn.c

eyes.c: $(srcdir)/eyes.db mkeyes$(EXEEXT)
	./mkeyes < $(srcdir)/eyes.db >eyes.c
