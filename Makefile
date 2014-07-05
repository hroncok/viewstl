INCLUDE = -I/usr/X11R6/include
LIBDIR  = -L/usr/X11/lib -L/usr/X11R6/lib
 
COMPILERFLAGS = -O2 -fomit-frame-pointer -g -pipe -Wall -fexceptions -fstack-protector-strong -grecord-gcc-switches -pedantic
CC = gcc
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
LIBRARIES = -lglut -lGL -lGLU -lX11 -lXext -lXmu -lXt -lXi -lm -ladmesh

all: viewstl 

viewstl: viewstl.c
	$(CC) $(CFLAGS) -o $@ $(LIBDIR) $< $(LIBRARIES) 

clean:
	rm viewstl

.PHONY: all clean
