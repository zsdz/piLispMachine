#---------------------------
# Makefile for GNUGO on UNIX
#
# Usage: make
#---------------------------

SRC = count.c \
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

PRG = gnugo

OBJ = $(SRC:.c=.o)

CFLAGS = -O

$(PRG) : $(OBJ)
	$(CC) $(OBJ) -o $@
	/bin/rm -f *.o

matchpat.o : patterns.h
$(OBJ) : gnugo.h
