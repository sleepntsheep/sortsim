CC ?= cc
CFLAGS += -std=c99 -Wall -Wextra -pedantic -O3 -g
LDFLAGS += -lSDL2 -lSDL2_ttf -lm

all: sortsim

sortsim: *.c *.h
	$(CC) $(CFLAGS) -o sortsim main.c $(LDFLAGS)

