CFLAGS = -Wall -Wextra -ggdb
LIBS = -lraylib -lm

all: example

example: example.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
