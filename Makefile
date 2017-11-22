#
# Makefile for the malloc lab driver
#
CC = gcc
CFLAGS = -Wall -pg -O2 -g -I.#-pg -O2 -g -I.
MM_C = mm.c

OBJS = mdriver.o mm.o memlib.o pagemap.o fsecs.o fcyc.o clock.o ftimer.o

all: mdriver

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS) -lm

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h pagemap.h
pagemap.o: pagemap.c pagemap.h
mm.o: $(MM_C) mm.h memlib.h
	$(CC) $(CFLAGS) -c -o mm.o $(MM_C)
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

clean:
	rm -f *~ *.o mdriver
