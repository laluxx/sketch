CC = gcc
CFLAGS = -Wall -Wextra -I/usr/include/cairo -I/usr/include/X11
LDFLAGS = -lX11 -lcairo
TARGET = sketch

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
