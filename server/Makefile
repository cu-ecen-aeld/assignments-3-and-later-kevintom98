CC?=gcc
CROSS_COMPILE=

CFLAGS ?= -Wall -Werror 
TARGET ?= aesdsocket
LDFLAGS ?= -pthread -lrt

all: aesdsocket

$(TARGET): $(TARGET).c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o$(TARGET) $(TARGET).c $(LDFLAGS)

clean:
	$(RM) $(TARGET)
