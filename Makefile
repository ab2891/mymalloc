# Agustin Blaumann (ab3211) -- CS 214 Project I
CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all test clean
all: memgrind memtest correctness

memgrind: memgrind.o mymalloc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
memtest: memtest.o mymalloc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
correctness: correctness.o mymalloc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

mymalloc.o memgrind.o memtest.o correctness.o: mymalloc.h

memtest-leak: memtest.c mymalloc.o mymalloc.h
	$(CC) $(CFLAGS) $(LDFLAGS) -DLEAK=1 -o $@ memtest.c mymalloc.o $(LDLIBS)

test: all memtest-leak
	sh check.sh

clean:
	rm -f *.o memgrind memtest memtest-leak correctness

