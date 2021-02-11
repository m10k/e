OBJECTS = main.o term.o config.o file.o buffer.o
OUTPUT = e
PHONY = clean

CFLAGS = -Wall -std=c99 -pedantic -fPIC
LIBS = -lncurses

all: $(OUTPUT)

clean:
	rm -rf $(OBJECTS) $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: $(PHONY)
