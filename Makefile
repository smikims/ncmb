CC	:= cc
SRC	:= ncmb.c
CFLAGS	:= -Wall -Wextra -pedantic-errors -ansi -lm -lcurses -std=c99
DFLAGS	:= -g -O0
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
