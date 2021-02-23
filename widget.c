#define _GNU_SOURCE
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include <errno.h>
#include "ui.h"

struct signal_handler {
	struct signal_handler *next;
	widget_handler_t *handler;
	void *user_data;
};

struct signal {
	struct signal *next;
	const char *name;
	struct signal_handler *handlers;
};

static int _widget_input(struct widget *widget, const int event)
{
	return(-ENOSYS);
}

static int _widget_resize(struct widget *widget)
{
	return(-ENOSYS);
}

static int _widget_redraw(struct widget *widget)
{
	return(-ENOSYS);
}

static int _widget_free(struct widget *widget)
{
	free(widget);
	return(0);
}

int widget_init(struct widget *widget)
{
	if(!widget) {
		return(-EINVAL);
	}

	widget->attrs = UI_ATTR_VISIBLE | UI_ATTR_EXPAND;

	widget->input = _widget_input;
	widget->resize = _widget_resize;
	widget->redraw = _widget_redraw;
	widget->free = _widget_free;

	return(0);
}

int widget_clear(struct widget *widget, const int x, const int y,
		 const int width, const int height)
{
        int cur_y;

        if(!widget) {
                return(-EINVAL);
        }

        for(cur_y = 0; cur_y < height; cur_y++) {
                int cur_x;

                for(cur_x = 0; cur_x < width; cur_x++) {
                        mvwaddch(widget->window,
				 widget->y + y + cur_y,
				 widget->x + x + cur_x,
				 ' ');
                }

		mvchgat(widget->y + y + cur_y,
			widget->x + x,
			width, 0, UI_COLOR_NORMAL, NULL);
        }

        return(0);
}

int widget_is_visible(struct widget *widget)
{
	if(!widget) {
		return(-EINVAL);
	}

	return(widget->attrs & UI_ATTR_VISIBLE);
}

int widget_set_visible(struct widget *widget, const int is_visible)
{
	if(!widget) {
		return(-EINVAL);
	}

	if(is_visible) {
		widget->attrs |= UI_ATTR_VISIBLE;
	} else {
		widget->attrs &= ~UI_ATTR_VISIBLE;
	}

	return(0);
}

int widget_add_signal(struct widget *widget, const char *name)
{
	struct signal *signal;

	if(!widget || !name) {
		return(-EINVAL);
	}

	signal = malloc(sizeof(*signal));

	if(!signal) {
		return(-ENOMEM);
	}

	memset(signal, 0, sizeof(*signal));

	signal->name = strdup(name);

	if(!signal->name) {
		free(signal);
		return(-ENOMEM);
	}

	signal->next = widget->signals;
	widget->signals = signal;

	return(0);
}

static int _signal_free(struct signal **sig)
{
	struct signal_handler *handler;

	if(!sig || !*sig) {
		return(-EINVAL);
	}

	handler = (*sig)->handlers;

	while(handler) {
		struct signal_handler *next;

		next = handler->next;
		free(handler);
		handler = next;
	}

	free(*sig);
	*sig = NULL;

	return(0);
}

int widget_remove_signal(struct widget *widget, const char *name)
{
	struct signal **cur;

	if(!widget || !name) {
		return(-EINVAL);
	}

	for(cur = &(widget->signals); *cur; cur = &(*cur)->next) {
		struct signal *next;

		if(strcmp((*cur)->name, name) != 0) {
			continue;
		}

		next = (*cur)->next;
		_signal_free(cur);
		*cur = next;
		return(0);
	}

	return(-ENOENT);
}

static int _widget_get_signal(struct widget *widget, const char *name, struct signal **signal)
{
	struct signal *sig;

	if(!widget || !name || !signal) {
		return(-EINVAL);
	}

	for(sig = widget->signals; sig; sig = sig->next) {
		if(strcmp(sig->name, name) == 0) {
			*signal = sig;
			return(0);
		}
	}

	return(-ENOENT);
}

int widget_add_handler(struct widget *widget, const char *name,
		       widget_handler_t *handler, void *user_data)
{
	struct signal_handler *sighandler;
	struct signal *signal;

	if(!widget || !name || !handler) {
		return(-EINVAL);
	}

	if(_widget_get_signal(widget, name, &signal) < 0) {
		return(-ENOENT);
	}

	sighandler = malloc(sizeof(*sighandler));

	if(!sighandler) {
		return(-ENOMEM);
	}

	memset(sighandler, 0, sizeof(*sighandler));

	sighandler->handler = handler;
	sighandler->user_data = user_data;

	/* FIXME: Really prepend? */
	sighandler->next = signal->handlers;
	signal->handlers = sighandler;

	return(0);
}

int widget_remove_handler(struct widget *widget, const char *name, widget_handler_t *handler)
{
	struct signal *signal;
	struct signal_handler **sighandler;

	if(!widget || !name || !handler) {
		return(-EINVAL);
	}

	if(_widget_get_signal(widget, name, &signal) < 0) {
		return(-ENOENT);
	}

	for(sighandler = &(signal->handlers); *sighandler; sighandler = &(*sighandler)->next) {
		struct signal_handler *next;

		if((*sighandler)->handler != handler) {
			continue;
		}

		next = (*sighandler)->next;
		free(*sighandler);
		*sighandler = next;
		return(0);
	}

	return(-ENOENT);
}

int widget_emit_signal(struct widget *widget, const char *name, void *data)
{
	struct signal *signal;
	struct signal_handler *handler;

	if(!widget || !name) {
		return(-EINVAL);
	}

	if(_widget_get_signal(widget, name, &signal) < 0) {
		return(-ENOENT);
	}

	for(handler = signal->handlers; handler; handler = handler->next) {
		if(handler->handler(widget, handler->user_data, data) < 0) {
			return(-ECANCELED);
		}
	}

	return(0);
}
