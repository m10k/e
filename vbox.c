#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"

struct vbox {
	struct container _parent;

	struct widget **children;
	size_t max_children;
	size_t num_children;
};

static int _vbox_input(struct widget *widget, const int event)
{
	struct vbox *vbox;
	size_t slot;

	if(!widget) {
		return(-EINVAL);
	}

	vbox = (struct vbox*)widget;

	for(slot = 0; slot < vbox->max_children; slot++) {
		if(vbox->children[slot]) {
			widget_input(vbox->children[slot], event);
		}
	}

	return(0);
}

static int _vbox_resize(struct widget *widget)
{
	struct vbox *vbox;

	if(!widget) {
		return(-EINVAL);
	}

	vbox = (struct vbox*)widget;

	if(vbox->num_children > 0) {
		int rem_height;
		int rem_children;
		int slot;

		rem_height = widget->height;
		rem_children = vbox->num_children;

		for(slot = 0; slot < vbox->max_children; slot++) {
			int height;

			if(!vbox->children[slot]) {
				continue;
			}

			/*
			 * To avoid losing pixels due to integer division,
			 * recalculate the remainder during each iteration.
			 */
			height = rem_height / rem_children;

			widget_set_size(vbox->children[slot],
					widget->width, height);
			widget_resize(vbox->children[slot]);

			rem_height -= height;
			rem_children--;
		}
	}

	widget_redraw(widget);

	return(0);
}

static int _vbox_redraw(struct widget *widget)
{
	struct vbox *vbox;

	if(!widget) {
		return(-EINVAL);
	}

	vbox = (struct vbox*)widget;

	if(vbox->num_children > 0) {
		int slot;

		for(slot = 0; slot < vbox->max_children; slot++) {
			if(vbox->children[slot]) {
				widget_redraw(vbox->children[slot]);
			}
		}
	}

	return(0);
}

static int _vbox_free(struct widget *widget)
{
	struct vbox *vbox;
	int slot;

	if(!widget) {
		return(-EINVAL);
	}

	vbox = (struct vbox*)widget;

	for(slot = 0; slot < vbox->max_children; slot++) {
		if(vbox->children[slot]) {
			widget_free(vbox->children[slot]);
		}
	}

	return(0);
}

static int _vbox_add(struct container *container, struct widget *child)
{
	struct vbox *vbox;
	size_t slot;

	if(!container || !child) {
		return(-EINVAL);
	}

	if(child->parent) {
		return(-EALREADY);
	}

	vbox = (struct vbox*)container;

	if(vbox->num_children >= vbox->max_children) {
		return(-EOVERFLOW);
	}

	for(slot = 0; slot < vbox->max_children; slot++) {
		if(!vbox->children[slot]) {
			vbox->children[slot] = child;
			break;
		}
	}

	vbox->num_children++;
	child->parent = (struct widget*)vbox;
	child->window = ((struct widget*)vbox)->window;

	widget_resize((struct widget*)vbox);

	return(0);
}

int vbox_new(struct vbox **dst, int size)
{
	struct vbox *vbox;

	if(!dst || size < 1) {
		return(-EINVAL);
	}

	vbox = malloc(sizeof(*vbox));

	if(!vbox) {
		return(-ENOMEM);
	}

	memset(vbox, 0, sizeof(*vbox));

	vbox->max_children = size;
	vbox->children = malloc(sizeof(*vbox->children) * size);

	if(!vbox->children) {
		free(vbox);
		return(-ENOMEM);
	}

	memset(vbox->children, 0, sizeof(*vbox->children) * size);

	((struct widget*)vbox)->attrs = UI_ATTR_EXPAND;

	((struct widget*)vbox)->input = _vbox_input;
	((struct widget*)vbox)->resize = _vbox_resize;
	((struct widget*)vbox)->redraw = _vbox_redraw;
	((struct widget*)vbox)->free = _vbox_free;
	((struct container*)vbox)->add = _vbox_add;

	*dst = vbox;

	return(0);
}
