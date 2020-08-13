# Makefile for vector field visualization tools

all: stplace stdraw mfield noise

LC = ../libs
INCLUDE = -I. -I$(LC) -I/usr/include/X11R5 -I/usr/include

# HP's
# LIBS = /usr/lib/X11R5/libX11.a -lm -lmalloc

# SGI's
LIBS = -lX11 -lm

#CFLAGS = -g $(INCLUDE)
CFLAGS = -O $(INCLUDE)

CC = cc

.cpp.o:
	 $(CC) $(CFLAGS) -c $*.cpp

STPLACE_C = stplace.cpp vfield.cpp streamline.cpp lowpass.cpp repel.cpp intersect.cpp \
  xlines.cpp dissolve.cpp visparams.cpp
STPLACE_OBJS = $(STPLACE_C:.cpp=.o) $(LC)/floatimage.o $(LC)/window.o \
               $(LC)/cli.o $(LC)/clip_line.o

stplace: $(STPLACE_OBJS) Makefile
	$(CC) $(CFLAGS) -o stplace $(STPLACE_OBJS) $(LIBS)

STDRAW_C = stdraw.cpp sd_vfield.cpp sd_streamline.cpp sd_repel.cpp sd_params.cpp \
           sd_picture.cpp
STDRAW_OBJS = $(STDRAW_C:.cpp=.o) $(LC)/floatimage.o $(LC)/window.o $(LC)/cli.o \
              $(LC)/clip_line.o

stdraw: $(STDRAW_OBJS) Makefile
	$(CC) $(CFLAGS) -o stdraw $(STDRAW_OBJS) $(LIBS)

MFIELD_C = mfield.cpp
MFIELD_OBJS = $(MFIELD_C:.cpp=.o)

mfield: $(MFIELD_OBJS) Makefile
	$(CC) $(CFLAGS) -o mfield $(MFIELD_OBJS) $(LIBS)

NOISE_C = noise.cpp
NOISE_OBJS = $(NOISE_C:.cpp=.o)

noise: $(NOISE_OBJS) Makefile
	$(CC) $(CFLAGS) -o noise $(NOISE_OBJS) $(LIBS)

vfield.o stplace.o streamline.o lowpass.o repel.o: vfield.h streamline.h
stplace.o repel.o: repel.h
lowpass.o: lowpass.h

sd_vfield.o stdraw.o sd_streamline.o sd_repel.o: stdraw.h
sd_vfield.o stdraw.o sd_streamline.o sd_repel.o: sd_vfield.h sd_streamline.h
stdraw.o sd_repel.o: sd_repel.h
stdraw.o sd_streamline.o: sd_params.h
stdraw.o sd_picture.o: sd_picture.h

