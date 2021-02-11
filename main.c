#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "term.h"
#include "buffer.h"

static void print_usage(const char *argv0)
{
	printf("Usage: %s filename\n", argv0);
	return;
}

int main(int argc, char *argv[])
{
	struct term *term;
	struct buffer *buffer;
	int err;
	char c;

	if(argc < 2) {
		print_usage(argv[0]);
		return(1);
	}

	err = term_new(&term);

	if(err < 0) {
		return(1);
	}

	err = buffer_open(&buffer, argv[1]);

	if(err < 0) {
		fprintf(stderr, "buffer_open: %s\n", strerror(-err));
		return(1);
	}

	printw("%s", buffer_get_data(buffer));
	refresh();

	while(read(STDIN_FILENO, &c, 1) == 1) {
		if(iscntrl(c)) {
			if(c == 17) {
				break;
			} else if(c == 19) {
				buffer_save(buffer);
			} else if(c == 13) {
				buffer_append(buffer, '\n');
				printw("\n");
			} else {
				printw("\\%d", c);
			}

			refresh();
		} else {
			buffer_append(buffer, c);

			printw("%c", c);
			refresh();
		}
	}

	err = buffer_close(&buffer);

	if(err < 0) {
		fprintf(stderr, "buffer_close: %s\r\n", strerror(-err));
	}

	err = term_destroy(&term);

	if(err < 0) {
		fprintf(stderr, "term_destroy: %s\r\n", strerror(-err));
	}

	return(err);
}
