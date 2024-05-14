SHELL=/bin/sh

all: writer

writer: writer.o
	$(CROSS_COMPILE)$(CC) -g -Wall -I/ writer.o -o writer

writer.o: writer.c
	$(CROSS_COMPILE)$(CC) -g -Wall -c -o writer.o writer.c

clean:
	rm -f writer writer.o
