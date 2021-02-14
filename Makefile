OBJECTS = main.o term.o config.o file.o buffer.o telex.o string.o window.o cmdbox.o
OUTPUT = e
PHONY = clean

CFLAGS = -Wall -std=c99 -pedantic -fPIC
LIBS = -lncurses

all: $(OUTPUT)

uitest: ui.o string.o window.o cmdbox.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf $(OBJECTS) $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: $(PHONY)
