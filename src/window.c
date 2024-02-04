#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include "ui.h"

struct window {
	struct container _parent;

	WINDOW *window;
	struct widget *child;
};

static int _get_terminal_size(int *width, int *height)
{
	struct winsize wsize;

	if(ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize) < 0) {
		return(-errno);
	}

	*width = wsize.ws_col;
	*height = wsize.ws_row;

	return(0);
}

int _window_input(struct widget *widget, const int ev)
{
	struct window *window;

	if(!widget) {
		return(-EINVAL);
	}

	window = (struct window*)widget;

	if(window->child) {
		widget_input(window->child, ev);
	}

	return(0);
}

int window_adjust_size(struct window *window)
{
	int width;
	int height;

	if(!window) {
		return(-EINVAL);
	}

	if(_get_terminal_size(&width, &height) < 0) {
		return(-EIO);
	}

	if(width == ((struct widget*)window)->width &&
	   height == ((struct widget*)window)->height) {
		return(0);
	}

	if(resizeterm(height, width) != OK) {
		return(-EIO);
	}

	widget_resize((struct widget*)window);
	widget_redraw((struct widget*)window);

	return(0);
}

int _window_resize(struct widget *widget)
{
	struct window *window;
	int max_width;
	int max_height;

	if(!widget) {
		return(-EINVAL);
	}

	window = (struct window*)widget;

	getmaxyx(stdscr, max_height, max_width);
	widget->width = max_width;
	widget->height = max_height;

	if(window->child) {
		window->child->x = 0;
		window->child->y = 0;
		window->child->width = widget->width;
		window->child->height = widget->height;

		widget_resize(window->child);
	}

	widget_redraw(widget);

	return(0);
}

int _window_redraw(struct widget *widget)
{
	struct window *window;

	if(!widget) {
		return(-EINVAL);
	}

	window = (struct window*)widget;

	if(window->child) {
		widget_redraw(window->child);
	}

	refresh();

	return(0);
}

int _window_free(struct widget *widget)
{
	struct window *window;

	if(!widget) {
		return(-EINVAL);
	}

	window = (struct window*)widget;

	if(window->child) {
		widget_free(window->child);
	}

	endwin();
	memset(window, 0, sizeof(*window));
	free(window);

	return(0);
}

int _window_add(struct container *container, struct widget *child)
{
	struct window *window;

	if(!container || !child) {
		return(-EINVAL);
	}

	window = (struct window*)container;

	if(window->child || child->parent) {
		return(-EALREADY);
	}

	child->parent = (struct widget*)window;
	child->window = window->window;
	window->child = child;

	widget_resize((struct widget*)window);
	widget_redraw((struct widget*)window);

	return(0);
}

static int _initialize_curses(void)
{
	static int _initialized = 0;
	int err;

	if(_initialized) {
		return(0);
	}

	if(!initscr()) {
		err = -EFAULT;
	} else if(start_color() == ERR) {
		err = -EIO;
	} else if(noecho() == ERR) {
	        err = -EIO;
	} else if(raw() == ERR) {
		err = -EIO;
	} else {
		err = 0;
		_initialized = 1;

		init_pair(UI_COLOR_NORMAL,
			  COLOR_BLACK, COLOR_WHITE);
		init_pair(UI_COLOR_LINES,
			  COLOR_WHITE, COLOR_BLUE);
		init_pair(UI_COLOR_SELECTION,
			  COLOR_WHITE, COLOR_BLUE);
		init_pair(UI_COLOR_DELETION,
			  COLOR_WHITE, COLOR_RED);
		init_pair(UI_COLOR_INSERTION,
			  COLOR_GREEN, COLOR_WHITE);
		init_pair(UI_COLOR_STATUS,
			  COLOR_BLACK, COLOR_GREEN);
		init_pair(UI_COLOR_COMMAND,
			  COLOR_WHITE, COLOR_BLACK);
	}

	if(err < 0 && err != -EFAULT) {
		endwin();
	}

	return(err);
}

int window_new(struct window **window)
{
	struct window *wind;
	int max_width;
	int max_height;

	if(!window) {
		return(-EINVAL);
	}

	if(_initialize_curses() < 0) {
		return(-EIO);
	}

	wind = malloc(sizeof(*wind));

	if(!wind) {
		return(-ENOMEM);
	}

	memset(wind, 0, sizeof(*wind));

	container_init((struct container*)wind);

	getmaxyx(stdscr, max_height, max_width);
	((struct widget*)wind)->width = max_width;
	((struct widget*)wind)->height = max_height;

	((struct widget*)wind)->input = _window_input;
	((struct widget*)wind)->resize = _window_resize;
	((struct widget*)wind)->redraw = _window_redraw;
	((struct widget*)wind)->free = _window_free;

	((struct container*)wind)->add = _window_add;

	wind->window = stdscr;
	*window = wind;

	return(0);
}
