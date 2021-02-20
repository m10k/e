#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"

struct window {
	struct container _parent;

	WINDOW *window;
	struct widget *child;
};

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

int _window_resize(struct widget *widget)
{
	struct window *window;

	if(!widget) {
		return(-EINVAL);
	}

	window = (struct window*)widget;

	if(window->child) {
		window->child->x = 0;
		window->child->y = 0;
		window->child->width = widget->width;
		window->child->height = widget->height;
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
	}

	if(err < 0 && err != -EFAULT) {
		endwin();
	}

	return(err);
}

int window_new(struct window **window)
{
	struct window *wind;

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

	((struct widget*)wind)->width = stdscr->_maxx;
	((struct widget*)wind)->height = stdscr->_maxy;

	((struct widget*)wind)->input = _window_input;
	((struct widget*)wind)->resize = _window_resize;
	((struct widget*)wind)->redraw = _window_redraw;
	((struct widget*)wind)->free = _window_free;

	((struct container*)wind)->add = _window_add;

	wind->window = stdscr;
	*window = wind;

	return(0);
}
