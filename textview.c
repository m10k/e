#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "ui.h"
#include "buffer.h"
#include "telex.h"

struct textview {
	struct widget _parent;

	struct buffer *buffer;
	struct telex *start;
	struct telex *end;
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
	int hl_start;
	int hl_end;
	const char *linepos;

	widget = (struct widget*)textview;

	abs_x = widget->x;
	abs_y = widget->y + y;
	lineno = line_get_number(line);
	/* width of the last line number to be displayed */
	lineno_width = _number_width(lineno + widget->height - y);
	text_width = widget->width - lineno_width - 1;

	remaining = line_get_length(line);
	linepos = line_get_data(line);

	while(remaining >= 0) {
		mvprintw(abs_y, abs_x, "%*d", lineno_width, lineno);
		mvchgat(abs_y, abs_x, lineno_width + 1, 0, UI_COLOR_LINES, NULL);

		mvprintw(abs_y, abs_x + lineno_width + 1, "%.*s", text_width, linepos);
		mvchgat(abs_y, abs_x + lineno_width + 1, -1, 0, UI_COLOR_NORMAL, NULL);

		linepos += text_width;
		remaining -= text_width;
		abs_y++;
	}

	if(line_get_highlight(line, &hl_start, &hl_end) == 0) {
		abs_y = widget->y + y;
		remaining = line_get_length(line);

		if(hl_start == 0) {
			hl_start = remaining;
		} else {
			hl_start--;
		}

		if(hl_end == 0) {
			hl_end = INT_MAX;
		} else {
			hl_end--;
		}

		while(remaining >= 0) {
			if(hl_start < text_width) {
				mvchgat(abs_y, abs_x + lineno_width + 1 + hl_start,
					hl_end > text_width ? -1 : hl_end, 0,
					UI_COLOR_SELECTION, NULL);
			} else {
				mvchgat(abs_y, abs_x + lineno_width + 1, hl_end, 0,
					UI_COLOR_SELECTION, NULL);
			}

			hl_start -= remaining;
			hl_end -= remaining;
			remaining -= text_width;
		}
	}

	return(0);
}

static int _textview_draw_status(struct textview *textview)
{
	struct widget *widget;
	char from[128];
	char to[128];
	char status[384];

	if(!textview) {
		return(-EINVAL);
	}

	widget = (struct widget*)textview;

	if(textview->start) {
		telex_to_string(textview->start, from, sizeof(from));
	} else {
		from[0] = 0;
	}

	if(textview->end) {
		telex_to_string(textview->end, to, sizeof(to));
	} else {
		to[0] = 0;
	}

	if(!textview->start && !textview->end) {
		status[0] = 0;
	} else {
		snprintf(status, sizeof(status), "Selection [ %s : %s ]",
			 from, to);
	}

	widget_clear(widget, 0, widget->height - 1, widget->width, 1);

	mvprintw(widget->y + widget->height - 1, 0, "%s", status);
	mvchgat(widget->y + widget->height - 1, 0, -1, 0, UI_COLOR_STATUS, NULL);

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
	max_lines = widget->height - 1;
	y = 0;

	/* FIXME: clear textview */

	if(textview->start) {
		err = buffer_get_snippet_telex(textview->buffer,
					       textview->start,
					       textview->end,
					       max_lines,
					       &snip);
	} else {
		err = buffer_get_snippet(textview->buffer, 1, max_lines, &snip);
	}

	if(err < 0) {
		return(err);
	}

	for(line = snippet_get_first_line(snip); line; line = line_get_next(line), y++) {
		_textview_draw_line(textview, y, line);
	}

	snippet_free(&snip);

	_textview_draw_status(textview);

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

	memset(view, 0, sizeof(*view));

	widget_init((struct widget*)view);

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
	int selection_changed;

	if(!textview) {
		return(-EINVAL);
	}

	if(textview->start != start || textview->end != end) {
		selection_changed = TRUE;
	}

	textview->start = start;
	textview->end = end;

	if(selection_changed) {
		widget_set_visible((struct widget*)textview, textview->start == NULL);
		widget_redraw((struct widget*)textview);
	}

	return(0);
}
