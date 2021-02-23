#include <ncurses.h>
#include <stdlib.h>
#include <errno.h>
#include "ui.h"

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
