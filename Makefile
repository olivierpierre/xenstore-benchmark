CC=gcc
CFLAGS=-Wall -Werror
LDFLAGS=-lm -lxenstore
PROG=xenstore-benchmark

all: $(PROG)

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(PROG)
