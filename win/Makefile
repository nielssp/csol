CC = gcc
CFLAGS = -Wall -pedantic -DUSE_PDCURSES -DUSE_GETOPT
LDFLAGS = -l:libpdcurses.a
CSOL_SRCDIR = ../src

SOURCES = $(wildcard $(CSOL_SRCDIR)/*.c)
OBJS = $(SOURCES:.c=.o)

all: csol.exe

csol.exe: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) csol.exe
