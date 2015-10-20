CROSS_COMPILE=arm-linux-gnueabi-
CC=gcc
CFLAGS=-static -lc
LIBRARY=-l :libmtdev.a

revent: revent.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) revent.c -o revent $(LIBRARY)

clean:
	rm -rf revent

.PHONY: clean
