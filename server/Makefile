CC=gcc
CROSS_COMPILE=

CFLAGS = -Wall -Werror
TARGET = aesdsocket


all: aesdsocket

$(TARGET): $(TARGET).c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)
