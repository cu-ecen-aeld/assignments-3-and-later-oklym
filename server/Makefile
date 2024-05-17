SHELL=/bin/sh

CFLAGS := -g -Wall
TARGET := aesdsocket

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -I/ -o $(TARGET) $(TARGET).o

$(TARGET).o: $(TARGET).c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c -o $(TARGET).o $(TARGET).c

clean:
	rm -f $(TARGET) $(TARGET).o
