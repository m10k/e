#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "ui.h"

int main(void)
{
	struct window *w;
	struct cmdbox *cb;
	int err;

	err = window_new(&w);

	if(err < 0) {
		fprintf(stderr, "window_new: %s\n", strerror(-err));
		return(1);
	}

	err = cmdbox_new(&cb);

	if(err < 0) {
		fprintf(stderr, "cmdbox_new: %s\n", strerror(-err));
		return(1);
	}

	err = window_set_child(w, (struct widget*)cb);

	if(err < 0) {
		fprintf(stderr, "window_set_child: %s\n", strerror(-err));
		return(1);
	}

	widget_redraw((struct widget*)w);

	while(1) {
		int ch;

		ch = getch();
		fprintf(stderr, "KEY[%d] = %s\n", ch, keyname(ch));

		if(widget_input((struct widget*)w, ch) == -EIO) {
			break;
		}
	}

	widget_free((struct widget*)w);
	return(0);
}
