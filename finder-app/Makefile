CC=gcc
CROSS_COMPILE=

CFLAGS = -Wall
TARGET = writer

$(TARGET): $(TARGET).c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)