CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror
LDFLAGS ?= -lpthread -lrt
TARGET ?= aesdsocket

.PHONY: all clean

all: $(TARGET)

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) $(LDFLAGS) -pthread -o $@ $<

default: all

clean:
	rm -f $(TARGET)
