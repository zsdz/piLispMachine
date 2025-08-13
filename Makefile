#
# Makefile
#

include ../../Config.mk

CIRCLEHOME = ../../libs/circle
NEWLIBDIR = ../../install/$(NEWLIB_ARCH)

#OBJS	= main.o util.o game/TicTacToe/TicTacToe.o $(wildcard game/gnugo/*.o) editor.o lisp.o kernel.o

#OBJS = main.o util.o game/TicTacToe/TicTacToe.o \
	$(wildcard game/gnugo-2.6Ori/utils/*.o) \
	$(wildcard game/gnugo-2.6Ori/sgf/*.o) \
	$(wildcard game/gnugo-2.6Ori/patterns/*.o) \
	$(wildcard game/gnugo-2.6Ori/engine/*.o) \
	$(wildcard game/gnugo-2.6Ori/interface/*.o) \
	editor.o lisp.o kernel.o

OBJS	= main.o util.o game/TicTacToe/TicTacToe.o \
	game/gnugo/count.o \
	game/gnugo/countlib.o \
	game/gnugo/endgame.o \
	game/gnugo/eval.o \
	game/gnugo/exambord.o \
	game/gnugo/findcolr.o \
	game/gnugo/findnext.o \
	game/gnugo/findopen.o \
	game/gnugo/findpatn.o \
	game/gnugo/findsavr.o \
	game/gnugo/findwinr.o \
	game/gnugo/fioe.o \
	game/gnugo/genmove.o \
	game/gnugo/getij.o \
	game/gnugo/getmove.o \
	game/gnugo/initmark.o \
	game/gnugo/matchpat.o \
	game/gnugo/opening.o \
	game/gnugo/openregn.o \
	game/gnugo/sethand.o \
	game/gnugo/showbord.o \
	game/gnugo/showinst.o \
	game/gnugo/suicide.o \
	game/gnugo/gnugo.o \
	editor.o lisp.o kernel.o

#OBJS	= main.o util.o game/TicTacToe/TicTacToe.o \
	game/gnugo2/utils/getopt.o \
	game/gnugo2/utils/getopt1.o \
	game/gnugo2/sgf/sgf_utils.o \
	game/gnugo2/sgf/sgf.o \
	game/gnugo2/sgf/sgfana.o \
	game/gnugo2/sgf/sgfgen.o \
	game/gnugo2/sgf/ttsgf_read.o \
	game/gnugo2/sgf/ttsgf_write.o \
	game/gnugo2/sgf/ttsgf.o \
	game/gnugo2/patterns/connections.o \
	game/gnugo2/patterns/helpers.o \
	game/gnugo2/patterns/joseki.o \
	game/gnugo2/patterns/mkeyes.o \
	game/gnugo2/patterns/mkpat.o \
	game/gnugo2/patterns/patterns.o \
	game/gnugo2/patterns/conn.o \
	game/gnugo2/patterns/eyes.o \
	game/gnugo2/engine/attdef.o \
	game/gnugo2/engine/dragon.o \
	game/gnugo2/engine/filllib.o \
	game/gnugo2/engine/fuseki.o \
	game/gnugo2/engine/genmove.o \
	game/gnugo2/engine/globals.o \
	game/gnugo2/engine/hash.o \
	game/gnugo2/engine/matchpat.o \
	game/gnugo2/engine/moyo.o \
	game/gnugo2/engine/optics.o \
	game/gnugo2/engine/reading.o \
	game/gnugo2/engine/semeai.o \
	game/gnugo2/engine/sethand.o \
	game/gnugo2/engine/shapes.o \
	game/gnugo2/engine/showbord.o \
	game/gnugo2/engine/utils.o \
	game/gnugo2/engine/globals.o \
	game/gnugo2/engine/worm.o \
	game/gnugo2/interface/gmp.o \
	game/gnugo2/interface/interface.o \
	game/gnugo2/interface/play_ascii.o \
	game/gnugo2/interface/play_gmp.o \
	game/gnugo2/interface/play_solo.o \
	game/gnugo2/interface/play_test.o \
	game/gnugo2/interface/gnugo2.6.o \
	editor.o lisp.o kernel.o

include $(CIRCLEHOME)/Rules.mk

CFLAGS += -I "$(NEWLIBDIR)/include" -I $(STDDEF_INCPATH) -I ../../include -Wno-write-strings

LIBS := "$(NEWLIBDIR)/lib/libm.a" "$(NEWLIBDIR)/lib/libc.a" "$(NEWLIBDIR)/lib/libcirclenewlib.a" \
 	$(CIRCLEHOME)/addon/SDCard/libsdcard.a \
  	$(CIRCLEHOME)/lib/usb/libusb.a \
 	$(CIRCLEHOME)/lib/input/libinput.a \
 	$(CIRCLEHOME)/addon/fatfs/libfatfs.a \
 	$(CIRCLEHOME)/lib/fs/libfs.a \
  	$(CIRCLEHOME)/lib/net/libnet.a \
  	$(CIRCLEHOME)/lib/sched/libsched.a \
  	$(CIRCLEHOME)/lib/libcircle.a

-include $(DEPS)
