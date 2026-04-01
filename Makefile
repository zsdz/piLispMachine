#
# Makefile
#

include ../../Config.mk

CIRCLEHOME = ../../libs/circle
NEWLIBDIR = ../../install/$(NEWLIB_ARCH)

#OBJS	= main.o util.o game/TicTacToe/TicTacToe.o $(wildcard game/gnugo/*.o) editor.o lisp.o kernel.o

#OBJS	= main.o util.o game/TicTacToe/TicTacToe.o \
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

GNUGODIR = game/gnugo
GNUGOSRC = $(wildcard $(GNUGODIR)/*.c)

OBJS = main.o util.o game/TicTacToe/TicTacToe.o $(GNUGOSRC:%.c=%.o) editor.o lisp.o kernel.o

include $(CIRCLEHOME)/Rules.mk

CFLAGS += -I "$(NEWLIBDIR)/include" -I $(STDDEF_INCPATH) -I ../../include -Wno-write-strings

#librustTest.a:
#	rustc --crate-type=staticlib --target=aarch64-unknown-none rustTest.rs

LIBS := "$(NEWLIBDIR)/lib/libm.a" "$(NEWLIBDIR)/lib/libc.a" "$(NEWLIBDIR)/lib/libcirclenewlib.a" \
 	$(CIRCLEHOME)/addon/SDCard/libsdcard.a \
  	$(CIRCLEHOME)/lib/usb/libusb.a \
 	$(CIRCLEHOME)/lib/input/libinput.a \
 	$(CIRCLEHOME)/addon/fatfs/libfatfs.a \
 	$(CIRCLEHOME)/lib/fs/libfs.a \
  	$(CIRCLEHOME)/lib/net/libnet.a \
  	$(CIRCLEHOME)/lib/sched/libsched.a \
  	$(CIRCLEHOME)/lib/libcircle.a \
#	librustTest.a

-include $(DEPS)
