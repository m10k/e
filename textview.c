#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"

struct textview {
	struct widget _parent;

	struct buffer *buffer;
/*	struct selection selection; */
};

static int _textview_input(struct widget *widget, const int event)
{
	return(0);
}

static int _textview_resize(struct widget *widget)
{
	return(0);
}

static int _textview_redraw(struct widget *widget)
{
	struct snippet *snip;
	int max_lines;
	int err;

	max_lines = widget->height;

	if(/* have selection */ 0) {

	} else {
		err = buffer_get_snippet(widget->buffer, 1, max_lines, &snip);

		if(err < 0) {
			return(err);
		}

		snippet_free(&snip);
	}

	return(0);
}

static int _textview_free(struct widget *widget)
{
	struct textview *textview;

	textview = (struct textview*)widget;

	memset(textview, 0, sizeof(*textview));
	free(textview);

	return(0);
}

int textview_new(struct textview **textview)
{
	struct textview *view;

	if(!textview) {
		return(-EINVAL);
	}

	view = malloc(sizeof(*view));

	if(!view) {
		return(-ENOMEM);
	}

	((struct widget*)view)->input = _textview_input;
	((struct widget*)view)->resize = _textview_resize;
	((struct widget*)view)->redraw = _textview_redraw;
	((struct widget*)view)->free = _textview_free;

	*textview = view;
	return(0);
}
