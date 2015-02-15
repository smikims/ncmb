CC	:= cc
SRC	:= ncmb.c
CFLAGS	:= -Wall -Wextra -pedantic-errors -ansi -lm -lcurses -std=c99
DFLAGS	:= -g -O0
INSTALL	:= /usr/local/bin/ncmb
OUT	:= ncmb

all:	$(OUT)

$(OUT):	$(SRC)
	$(CC) $(CFLAGS) -O3 -o $@ $?

clean:
	rm -f $(OUT) gmon.out

debug:	$(SRC)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(OUT) $?

install: $(OUT)
	install $(OUT) $(INSTALL)
	strip $(INSTALL)
