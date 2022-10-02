CC ?= gcc
CFLAGS += -std=c99 -Wall -Wextra -pedantic -O3 -g
LDFLAGS += -lSDL2 -lSDL2_ttf

all: sortsim

sortsim: *.c *.h
	$(CC) $(CFLAGS) -o sortsim *.c $(LDFLAGS)

