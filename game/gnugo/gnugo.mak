#-------------------------------------
# Makefile for GNUGO on IBM PC using 
# Microsoft C/C++ Compiler and Linker
#
# Usage: nmake -f gnugo.mak
#-------------------------------------

SOURCES = count.c \
	  countlib.c \
	  endgame.c \
	  eval.c \
	  exambord.c \
	  findcolr.c \
	  findnext.c \
	  findopen.c \
	  findpatn.c \
	  findsavr.c \
	  findwinr.c \
	  fioe.c \
	  genmove.c \
	  getij.c \
	  getmove.c \
	  initmark.c \
	  main.c \
	  matchpat.c \
	  opening.c \
	  openregn.c \
	  sethand.c \
	  showbord.c \
	  showinst.c \
	  suicide.c

OBJECTS = $(SOURCES:.c=.obj)

gnugo.exe: $(OBJECTS)
	LINK @objs,gnugo.exe;
        del *.obj

matchpat.obj: patterns.h
$(OBJECTS): gnugo.h
