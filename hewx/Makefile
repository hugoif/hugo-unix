# Makefile for hewx
# wxWindows port of Hugo by Kent Tessman (c) 2001
# GTK 2 support improved by Tim Mann
#
# If compiling with wxGTK with GTK 1.2 support, search for "GTK 1.2"
# and uncomment the indicated lines.
#
# If compiling with wxGTK with GTK 2 support, search for "GTK 2"
# and uncomment the indicated lines.

# These may have to be changed depending on where things are installed:
# (The wxWindows file locations should really be taken from wx-config.)
HUGOSOURCEDIR = ../source
MIKMODINCLUDEDIR = /usr/local/include
CXXINCLUDEDIR = /usr/include/g++-3

# Libraries to be linked:
# If a working libmikmod.so has been compiled on the runtime system
# and the wxGTK library is installed:
STATICLIBS =

# *** GTK 1.2 vs. GTK 2: ***
# Uncomment the following if compiling with GTK 1.2 support:
#LDFLAGS = `wx-config --libs` -lgtk -lgdk -lmikmod
# Uncomment the following if compiling with GTK 2 support:
LDFLAGS = `wx-config --libs` -lgtk-x11-2.0 -lgdk-x11-2.0 -lmikmod

# Use these LDFLAGS and STATICLIBS for static building:
#STATICLIBS = /usr/local/lib/libmikmod.a /usr/local/lib/libwx_gtk2-2.4.a
#LDFLAGS = -lstdc++ -lglib-2.0 -lgthread-2.0 -lgtk-x11-2.0 -lgdk-x11-2.0 -lpthread -ljpeg

# Useful directories

MYCODEDIR := .

# Directories to search for header files

# *** GTK 1.2 vs. GTK 2: ***
# Uncomment the following if compiling with GTK 1.2 support:
#SEARCHDIRS := -I- -I${MYCODEDIR} -I${HUGOSOURCEDIR} -I${MIKMODINCLUDEDIR} -I${CXXINCLUDEDIR}
# Uncomment the following if compiling with GTK 2 support:
SEARCHDIRS := -I- -I${MYCODEDIR} -I${HUGOSOURCEDIR} -I${MIKMODINCLUDEDIR} -I${CXXINCLUDEDIR} -I/usr/include/gtk-2.0


# Hopefully nothing below here will need to be changed:

# makemake variables

LINKER       := gcc
DEPENDFLAGS  := ${SEARCHDIRS}
TOUCHHEADERS := ${MYCODEDIR}/*.h

# C

CC     := gcc
# Debugging:
#CFLAGS  = ${DEPENDFLAGS} -D__WXGTK__ -DWXWINDOWS -DNO_WXCARET -g3 -Wall `wx-config --cflags`
# Release build:
CFLAGS  = ${DEPENDFLAGS} -D__WXGTK__ -DWXWINDOWS -DNO_WXCARET -O2 -Wall `wx-config --cflags`

# C++

CXX      := g++
#CXXFLAGS  = ${DEPENDFLAGS}
CXXFLAGS = ${CFLAGS} -fno-rtti

%.o : %.cpp
	${CXX} ${CPPFLAGS} -c $< ${CXXFLAGS}

%.o : %.cxx
	${CXX} ${CPPFLAGS} -c $< ${CXXFLAGS}

# C preprocessor

CPPFLAGS = 

# linker

LOADLIBES   := -lm $(STATICLIBS)



# This is what makemake added


# ./hewx

./hewx : ../source/he.o ../source/hebuffer.o ../source/heexpr.o ../source/hemisc.o ../source/heobject.o ../source/heparse.o ../source/heres.o ../source/herun.o ../source/heset.o ../source/stringfn.o ./hewx.o ./hewxwin.o ./hejpeg.o ./hepath.o ./hesound.o ./hevideo.o
	${LINKER} ${LDFLAGS} -o $@ ${filter-out %.a %.so, $^} ${LOADLIBES}

# target for making everything

.PHONY : all
all: ./hewx


# target for removing all object files

.PHONY : tidy
tidy::
	@${RM} core ../source/he.o ../source/hebuffer.o ../source/heexpr.o ./hejpeg.o ../source/hemisc.o ../source/heobject.o ../source/heparse.o ./hepath.o ../source/heres.o ../source/herun.o ../source/heset.o ./hesound.o ./hevideo.o ./hewx.o ./hewxwin.o ../source/stringfn.o

# target for removing all object files

.PHONY : clean
clean:: tidy
	@${RM} ./hewx

# list of all source files

MM_ALL_SOURCES := ../source/he.c ../source/hebuffer.c ../source/heexpr.c ./hejpeg.cpp ../source/hemisc.c ../source/heobject.c ../source/heparse.c ./hepath.c ../source/heres.c ../source/herun.c ../source/heset.c ./hesound.cpp ./hevideo.cpp ./hewx.cpp ./hewxwin.cpp ../source/stringfn.c


# target for checking a source file

CHECKSYNTAXFILE := ${basename ${filter %${CHECKSTRING}, ${MM_ALL_SOURCES}}}

.PHONY : checksyntax
checksyntax:
  ifneq (${CHECKSYNTAXFILE},)
	@${MAKE} ${addsuffix .o, ${CHECKSYNTAXFILE}}
  else
	@echo No target to make ${CHECKSTRING}
  endif


# target for touching appropriate source files

.PHONY : touch
touch::
	@list=$$(grep -l ${TOUCHSTRING} ${MM_ALL_SOURCES}); \
     for file in $$list; do { echo $$file; touch $$file; } done


# target for calculating dependencies (MAKEMAKE)

.PHONY : jdepend
jdepend:
	@${MAKEMAKE} --depend Makefile -- ${DEPENDFLAGS} --  ../source/he.c ../source/he.o ../source/hebuffer.c ../source/hebuffer.o ../source/heexpr.c ../source/heexpr.o ./hejpeg.cpp ./hejpeg.o ../source/hemisc.c ../source/hemisc.o ../source/heobject.c ../source/heobject.o ../source/heparse.c ../source/heparse.o ./hepath.cpp ./hepath.o ../source/heres.c ../source/heres.o ../source/herun.c ../source/herun.o ../source/heset.c ../source/heset.o ./hesound.cpp ./hesound.o ./hevideo.cpp ./hevideo.o ./hewx.cpp ./hewx.o ./hewxwin.cpp ./hewxwin.o ../source/stringfn.c ../source/stringfn.o


# DO NOT DELETE THIS LINE -- makemake depends on it.

../source/he.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

../source/hebuffer.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

../source/heexpr.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

./hejpeg.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h ./hewx.h

../source/hemisc.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

../source/heobject.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

../source/heparse.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

../source/heres.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

../source/herun.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

../source/heset.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h

./hesound.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h ./hewx.h

./hewx.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h ./hewx.h

./hewxwin.o: ../source/hdinter.h ../source/heheader.h ../source/htokens.h ./hewx.h

../source/stringfn.o:  ../source/heheader.h

