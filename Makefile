OBJECTS = src/main.o src/config.o src/file.o src/buffer.o src/string.o src/kbdwidget.o \
	  src/window.o src/cmdbox.o src/editor.o src/vbox.o src/textview.o src/widget.o \
	  src/container.o
OUTPUT = e
PHONY = clean

CFLAGS = -Wall -pedantic -fPIC
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
