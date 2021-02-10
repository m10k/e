#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "term.h"

static void print_usage(const char *argv0)
{
	printf("Usage: %s filename\n", argv0);
	return;
}

int main(int argc, char *argv[])
{
	struct term *term;
	const char *filepath;
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

	filepath = argv[1];
	err = 0;

	while(read(STDIN_FILENO, &c, 1) == 1) {
		if(iscntrl(c)) {
			printf("%d\r\n", c);
		} else {
			printf("%d (%c)\r\n", c, c);
		}

		if(c == 'q') {
			break;
		}

	}

	err = term_destroy(&term);

	if(err < 0) {
		fprintf(stderr, "term_destroy: %s\n", strerror(-err));
	}

	return(err);
}
