#include <ncurses.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "file.h"

int main(int argc, char *argv[])
{
	struct window *window;
	struct textview *textview;
	struct buffer *buffer;
	int event;
	const char *path;
	const char *start_expr;
	const char *end_expr;
	struct telex *start;
	struct telex *end;
	const char *err_ptr;
	int err;

	if(argc < 4) {
		printf("Usage: %s file expr0 expr1\n", argv[0]);
		return(1);
	}

	path = argv[1];
	start_expr = argv[2];
	end_expr = argv[3];

	err = buffer_open(&buffer, path);

	if(err < 0) {
		fprintf(stderr, "buffer_open: %s\n", strerror(-err));
		return(1);
	}

	err = telex_parse(start_expr, &start, &err_ptr);

	if(err < 0) {
		fprintf(stderr, "telex_parse: Parser error near `%s'\n", err_ptr);
		return(1);
	}

	err = telex_parse(end_expr, &end, &err_ptr);

	if(err < 0) {
		fprintf(stderr, "telex_parse: Parser error near `%s'\n", err_ptr);
		return(1);
	}

	assert(window_new(&window) == 0);
	assert(textview_new(&textview) == 0);
	assert(textview_set_buffer(textview, buffer) == 0);
	textview_set_selection(textview, start, end);

	assert(window_set_child(window, (struct widget*)textview) == 0);

	widget_redraw((struct widget*)window);

	while((event = getch())) {
		if(event == 17) {
			break;
		}

		widget_input((struct widget*)window, event);
	}

	widget_free((struct widget*)window);

	return(0);
}
