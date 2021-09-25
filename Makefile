# Hugo makefile for GCC and Unix by Bill Lash, modified by Kent Tessman
#
# (Anything really ugly in this makefile has come about since I
# got my hands on it.  --KT)
#
# Bill's (modified) notes:
#
# Make the executables hc, he, and hd by (editing this appropriately and)
# typing 'make'.
#
# Two of the features of this version of Hugo may cause some problems on
# some terminals.  These features are "special character support" and
# "color handling".
#
# "Special character support" allows some international characters to be
# printed, (e.g. grave-accented characters, characters with umlauts, ...).
# Unfortunately it is was beyond my ability to determine if a particular
# terminal supports these characters.  For example, the xterm that I use
# at work under SunOS 4.1.3 does not support these characters in the default
# "fixed" font family, but if started with xterm -fn courier, to use the
# "courier" font family, it does support these characters.  On my Linux box
# at home, the "fixed" font does support these characters.
#
# "Color handling" allows the game to control the color of text.
# This can only be enabled if using ncurses.  The color handling works pretty
# well on the Linux console, but in a color xterm and setting the TERM
# environment variable to xterm-color, annoying things can sometimes happen.
# I am not sure if the problem lies in the code here, the ncurses library,
# xterm, or the terminfo entry for xterm-color.  Using color rxvt for the
# terminal emulator produces the same results so I suspect the code here, ncurses
# or the terminfo entry.  If the TERM environment variable is xterm, no
# color will be used (although "bright" colors will appear bold).
#
# Note: After looking into this further, it seems that if one runs a
#	color xterm and sets the TERM environment variable to pcansi,
#	the color handling works very well (assuming that you use a
#	background color of black and a foreground color of white or gray).
#	The command line to do this is:
#		color_xterm -bg black -fg white -tn pcansi
#	or
#		xterm-color -bg black -fg white -tn pcansi
#
# The following defines set configuration options:
#	DO_COLOR	turns on color handling (requires ncurses)
CFG_OPTIONS=-DDO_COLOR

# You can enable audio support in the CLI interpreter ("he".) To build with
# audio enabled, you'll need to have pkg-config installed, as well as the
# development files for the following libraries:
#
#   SDL 2
#   libfluidsynth (2.0 or higher; 1.x will NOT work)
#   libmpg123
#   libsndfile
#   libopenmpt, libmodplug or Libxmp (depending on the chosen MOD backend below)
#
# Set this to "yes" to build the CLI interpreter with audio support.
ENABLE_AUDIO?=no

# MOD music format backend (if audio support was enabled above.) Choices are:
#     mpt     - Uses libopenmpt. Best choice for accuracy and sound quality.
#     xmp     - Uses Libxmp. Better than modplug.
#     modplug - Uses libmodplug. Widely available, but not as good.
MOD_BACKEND?=mpt

# Define your optimization flags.  Most compilers understand -O and -O2,
# Debugging
#CFLAGS=-Wall -g 
#CXXFLAGS=-Wall -g
# Standard optimizations
CFLAGS?=-pipe -O2 -Wall -Wextra -pedantic
CXXFLAGS?=-pipe -O2 -Wall -Wextra -pedantic

# If you are using the ncurses package, define NCURSES for the compiler
CPPFLAGS:=$(CPPFLAGS) $(CFG_OPTIONS) -DNCURSES -DGCC_UNIX -DUSE_TEMPFILES

ifeq ($(MAKECMDGOALS), he)
    CPPFLAGS+=-DNO_LATIN1_CHARSET
endif

ifeq ($(MAKECMDGOALS), hd)
    CPPFLAGS+=-DDEBUGGER -DFRONT_END -DNO_LATIN1_CHARSET
endif

# If you need a special include path to get the right curses header, specify
# it.
INCLUDES+=-I/usr/local/include -Isource -I.

# C compiler. Usually already set to "cc" by make or the environment, but we're
# setting it here if not already set.
CC?=cc

# Ditto for the C++ compiler.
CXX?=c++

# Uncomment and edit these if you want to force the compiler to be something
# else. Usually there's no need to, as you can do that on the command line when
# calling make (e.g. "CC=gcc CXX=g++ make").
#CC=gcc
#CXX=g++

# If using ncurses you need -lncurses, otherwise use -lcurses.  You may also
# need -ltermcap or -ltermlib.  If you need to specify a particular path to
# the curses library, do so here.
#HE_LIBS+=-L/usr/lib -lcurses
#HE_LIBS+=-lcurses -ltermcap
#HE_LIBS+=-lcurses -ltermlib
#HE_LIBS+=-lcurses
#HE_LIBS+=-lncurses
#HE_LIBS+=-lncurses -ltinfo
# We ask pkg-config for the correct flags. If you don't have pkg-config, you
# need to comment this out and uncomment one of the above instead.
HE_LIBS+=$(shell pkg-config --libs ncurses)

# Shouldn't need to change anything below here.
SOURCE = source
HC_H=$(SOURCE)/hcheader.h $(SOURCE)/htokens.h
HE_H=$(SOURCE)/heheader.h
HD_H=$(HE_H) $(SOURCE)/hdheader.h $(SOURCE)/hdinter.h $(SOURCE)/htokens.h
HC_OBJS = hc.o hcbuild.o hccode.o hcdef.o hcfile.o hclink.o \
    hcmisc.o hccomp.o hcpass.o hcres.o stringfn.o hcgcc.o
HE_OBJS = he.o heexpr.o hemisc.o heobject.o heparse.o herun.o \
    heres.o heset.o stringfn.o hegcc.o
HD_OBJS = hd.o hddecode.o hdmisc.o hdtools.o hdupdate.o \
    hdval.o hdwindow.o hdgcc.o

ifeq ($(ENABLE_AUDIO), yes)
    # The audio code needs at least C++17.
    CXXFLAGS+=-std=c++17

    AULIB_OBJS += soundfont_data.o resample.o Decoder.o DecoderFluidsynth.o \
        DecoderMpg123.o DecoderSndfile.o Resampler.o ResamplerSpeex.o \
        Stream.o stream_p.o aulib.o sampleconv.o rwopsbundle.o audio.o

    AULIB_CPPFLAGS+= \
	-DAULIB_STATIC_DEFINE \
        -DOUTSIDE_SPEEX \
        -DRANDOM_PREFIX=SDL_audiolib \
        -DSOUND_AULIB \
        -DSOUND_SUPPORTED

    AULIB_INCLUDES+= \
	-ISDL_audiolib/3rdparty/speex_resampler \
	-ISDL_audiolib/include \
	-ISDL_audiolib/src \
	-Iaudio \
        -ISDL_audiolib \
        $(shell pkg-config --cflags sdl2 sndfile libmpg123 fluidsynth)

    AULIB_LIBS+=$(shell pkg-config --libs sdl2 sndfile libmpg123 fluidsynth)

    ifeq ($(MOD_BACKEND), mpt)
        AULIB_CPPFLAGS += -DUSE_DEC_OPENMPT=1
        AULIB_INCLUDES += $(shell pkg-config --cflags libopenmpt)
        AULIB_OBJS += DecoderOpenmpt.o
        AULIB_LIBS += $(shell pkg-config --libs libopenmpt)
    else ifeq ($(MOD_BACKEND), xmp)
        AULIB_CPPFLAGS += -DUSE_DEC_XMP=1
        AULIB_INCLUDES += $(shell pkg-config --cflags libxmp)
        AULIB_OBJS += DecoderXmp.o
        AULIB_LIBS += $(shell pkg-config --libs libxmp)
    else ifeq ($(MOD_BACKEND), modplug)
        AULIB_CPPFLAGS += -DUSE_DEC_MODPLUG=1
        AULIB_INCLUDES += $(shell pkg-config --cflags libmodplug)
        AULIB_OBJS += DecoderModplug.o
        AULIB_LIBS += $(shell pkg-config --libs libmodplug)
    endif

    HE_COMPILER:=$(CXX)
else
    HE_COMPILER:=$(CC)
endif

ifeq ($(MAKECMDGOALS), he)
    INCLUDES := $(INCLUDES) $(AULIB_INCLUDES)
    CPPFLAGS:=$(CPPFLAGS) $(AULIB_CPPFLAGS)
    HE_LIBS:=$(HE_LIBS) $(AULIB_LIBS)
endif

.PHONY: all clean

#all: hc he iotest
all: clean
	$(MAKE) -f $(MAKEFILE_LIST) hc
	$(RM) he*.o
	$(MAKE) -f $(MAKEFILE_LIST) he
	$(RM) he*.o
	$(MAKE) -f $(MAKEFILE_LIST) hd

clean:
	$(RM) $(HC_OBJS) $(HD_OBJS) $(HE_OBJS) hc he hd
ifeq ($(ENABLE_AUDIO), yes)
	$(RM) $(AULIB_OBJS) file2src file2src.o soundfont_data.h soundfont_data.cpp
endif

hc:	$(HC_OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o hc $(HC_OBJS)

he: $(AULIB_OBJS) $(HE_OBJS)
	$(HE_COMPILER) -o he $(HE_OBJS) $(AULIB_OBJS) $(HE_LIBS)

hd:	$(HE_OBJS) $(HD_OBJS)
	$(CC) $(CFLAGS) -o hd $(HE_OBJS) $(HD_OBJS) $(HE_LIBS)

iotest:	$(SOURCE)/iotest.c gcc/hegcc.c $(HE_H)
	$(CC) $(CFLAGS) -o iotest $(SOURCE)/iotest.c hegcc.o stringfn.o $(HE_LIBS)

file2src: file2src.o
	$(CXX) $(CXXFLAGS) -o $@ $<

soundfont_data.cpp: file2src
	./file2src audio/soundfont.sf2 soundfont_data soundfont_data

# Dep for not breaking parallel make.
audio.o: soundfont_data.cpp

%.o: gcc/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ $<

%.o: $(SOURCE)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ $<

%.o: SDL_audiolib/3rdparty/speex_resampler/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ $<

%.o: SDL_audiolib/src/%.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ $<

%.o: audio/%.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ $<

%.o: audio/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ $<
