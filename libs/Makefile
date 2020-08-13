# these are my C++ libraries
CFLAGS=-std=c95
LIBS = -lm /usr/lib/X11R5/libX11.a
INCLUDE = -I. -I/usr/include/X11R5 -I/usr/include
#CFLAGS = -g $(INCLUDE)
CFLAGS = -O $(INCLUDE)

CC = cc

.cpp.o:
	 $(CC) $(CFLAGS) -c $*.cpp

ALL_C = window.cpp floatimage.cpp cli.cpp clip_line.cpp
ALL_OBJS = $(ALL_C:.cpp=.o)

all: $(ALL_OBJS) Makefile

