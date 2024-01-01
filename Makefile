OBJECTS = main.o config.o file.o buffer.o string.o \
	  window.o cmdbox.o editor.o vbox.o textview.o widget.o \
	  container.o
OUTPUT = e
PHONY = clean

CFLAGS = -Wall -std=c99 -pedantic -fPIC
LIBS = -lncurses -ltelex

all: $(OUTPUT)

uitest: ui.o string.o window.o cmdbox.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

textview: config.o string.o telex.o file.o config.o textview.o window.o buffer.o textview_test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

telex: telex.o file.o string.o config.o telex_test.o
	$(CC) $(CFLAGS) -o $@ $^

snippet: telex.o file.o string.o config.o buffer.o snippet_test.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(OBJECTS) $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: $(PHONY)
