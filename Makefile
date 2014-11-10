CC	:= cc
SRC	:= mandel.c
CFLAGS	:= -Wall -pedantic-errors -ansi -lm -lcurses -std=c11
DFLAGS	:= -g -pg -O0
INSTALL	:= /usr/local/bin/ncmb
OUT	:= ncmb

all:	std

std:	$(SRC)
	$(CC) $(CFLAGS) -O3 -o $(OUT) $?

clean:	$(SRC)
	rm -f $(OUT) gmon.out

debug:	$(SRC)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(OUT) $?

install: std
	install $(OUT) $(INSTALL)
	strip $(INSTALL)
