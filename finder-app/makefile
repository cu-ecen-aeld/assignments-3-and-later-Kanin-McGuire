CC = $(CROSS_COMPILE)gcc 
CFLAGS = -Wall -Werror
TARGET = writer

.PHONY: all clean

all: $(TARGET)

writer: writer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
