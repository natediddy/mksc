CC = gcc
CFLAGS = -Wall -O2 -march=native -mtune=native
TARGET = mksc

SOURCES = mksc.c
OBJECTS = mksc.o

prefix = /usr/local

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

mksc.o:
	$(CC) $(CFLAGS) -c mksc.c

clean:
	@rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) $(prefix)/bin/$(TARGET)

uninstall:
	rm -f $(prefix)/bin/$(TARGET)

.PHONY: clean install uninstall
