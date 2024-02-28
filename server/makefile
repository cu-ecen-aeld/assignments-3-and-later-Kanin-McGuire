CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror
TARGET = aesdsocket

.PHONY: all clean

all: $(TARGET)

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o $@ $<

default: all

clean:
	rm -f $(TARGET)
