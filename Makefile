CC=clang
CFLAGS=-O2 -Wall -g
LIBS=-framework Foundation -framework CoreMIDI -framework AudioToolbox -lncurses -lpthread

all: miditest

output.o: output.c output.h
	$(CC) $(CFLAGS) -c output.c

input.o: input.c input.h
	$(CC) $(CFLAGS) -c input.c

harmony.o: harmony.c harmony.h
	$(CC) $(CFLAGS) -c harmony.c

samples.o: samples.c samples.h
	$(CC) $(CFLAGS) -c samples.c

miditest: miditest.c midi.h output.o input.o harmony.o samples.o
	$(CC) $(CFLAGS) $(LIBS) output.o input.o harmony.o samples.o miditest.c -o miditest


