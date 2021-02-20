#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"
#include "buffer.h"
#include "telex.h"

struct textview {
	struct widget _parent;

	struct buffer *buffer;
	struct telex *start;
	struct telex *end;
/*	struct selection selection; */
};

static int _number_width(int number)
{
	int width;

	for(width = 1; number > 0; width++) {
		number /= 10;
	}

	return(width);
}

static int _textview_input(struct widget *widget, const int event)
{
	return(0);
}

static int _textview_resize(struct widget *widget)
{
	return(0);
}

static int _textview_draw_line(struct textview *textview, const int y, struct line *line)
{
	struct widget *widget;
	int abs_x;
	int abs_y;

	int lineno;
	int lineno_width;
	int text_width;
	int remaining;
	const char *linepos;

	widget = (struct widget*)textview;

	abs_x = widget->x;
	abs_y = widget->y + y;
	lineno = line_get_number(line);
	lineno_width = _number_width(lineno);

	if(lineno_width < 4) {
		lineno_width = 4;
	}
	text_width = widget->width - lineno_width - 1;

	remaining = line_get_length(line);
	linepos = line_get_data(line);

	while(remaining >= 0) {
		mvprintw(abs_y, abs_x, "%*d ", lineno_width, lineno);
		mvprintw(abs_y, abs_x + lineno_width + 1, "%.*s", text_width, linepos);

		linepos += text_width;
		remaining -= text_width;
		abs_y++;
	}

	return(0);
}

static int _textview_redraw(struct widget *widget)
{
	struct textview *textview;
	struct snippet *snip;
	struct line *line;
	int max_lines;
	int err;
	int y;

	if(!widget) {
		return(-EINVAL);
	}

	textview = (struct textview*)widget;
	max_lines = widget->height;

	/* FIXME: clear textview */

	if(textview->start) {
		err = buffer_get_snippet_telex(textview->buffer,
					       textview->start, textview->end, &snip);
	} else {
		err = buffer_get_snippet(textview->buffer, 1, max_lines, &snip);
	}

	if(err < 0) {
		return(err);
	}

	for(y = 0, line = snippet_get_first_line(snip); line; line = line_get_next(line), y++) {
		_textview_draw_line(textview, y, line);
	}

	snippet_free(&snip);

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

int textview_set_buffer(struct textview *textview, struct buffer *buffer)
{
	if(!textview || !buffer) {
		return(-EINVAL);
	}

	textview->buffer = buffer;
	return(0);
}

int textview_set_selection(struct textview *textview, struct telex *start, struct telex *end)
{
	if(!textview) {
		return(-EINVAL);
	}

	textview->start = start;
	textview->end = end;

	return(0);
}
