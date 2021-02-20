#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"

struct vbox {

};

static int _vbox_input(struct widget *widget, const int event)
{
	return(0);
}

static int _vbox_resize(struct widget *widget)
{
	return(0);
}

static int _vbox_redraw(struct widget *widget)
{
	return(0);
}

static int _vbox_free(struct widget *widget)
{
	return(0);
}

static int _vbox_add(struct container *container, struct widget *child)
{
	return(0);
}

int vbox_new(struct vbox **dst)
{
	struct vbox *vbox;

	if(!dst) {
		return(-EINVAL);
	}

	vbox = malloc(sizeof(*vbox));

	if(!vbox) {
		return(-ENOMEM);
	}

	((struct widget*)vbox)->attrs = UI_ATTR_EXPAND;

	((struct widget*)vbox)->input = _vbox_input;
	((struct widget*)vbox)->resize = _vbox_resize;
	((struct widget*)vbox)->redraw = _vbox_redraw;
	((struct widget*)vbox)->free = _vbox_free;
	((struct container*)vbox)->add = _vbox_add;

	*dst = vbox;

	return(0);
}
