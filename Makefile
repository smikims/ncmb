CC      := cc
SRC     := ncmb.c
CFLAGS  := -std=c18 -Wall -Wextra -Wpedantic
DFLAGS  := -g -O0
LFLAGS  := -lm -lncursesw
INSTALL := /usr/local/bin/ncmb
OUT     := ncmb

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -O3 -o $@ $? $(LFLAGS)

clean:
	rm -f $(OUT) gmon.out

debug:	$(SRC)
	$(CC) $(CFLAGS) $(DFLAGS) -o $(OUT) $? $(LFLAGS)

install: $(OUT)
	install $(OUT) $(INSTALL)
	strip $(INSTALL)
