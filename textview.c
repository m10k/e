#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "ui.h"
#include "buffer.h"
#include "telex.h"
#include "config.h"

struct textview {
	struct widget _parent;

	struct buffer *buffer;
	struct telex *start;
	struct telex *end;

	int tab_width;

	int first_line;
	int pos_x;
	int pos_y;
	int num_width;
	int text_width;
	int cur_line;
};

static int _number_width(int number)
{
	int width;

	for(width = 1; number > 0; width++) {
		number /= 10;
	}

	return(width);
}

static int _textview_reset(struct textview *textview, const int first_line)
{
	int last_line;

	if(!textview) {
		return(-EINVAL);
	}

	textview->first_line = first_line;
	textview->cur_line = first_line;
	last_line = first_line + ((struct widget*)textview)->height - 1;

	textview->num_width = _number_width(last_line) + 1;
	textview->text_width = ((struct widget*)textview)->width - textview->num_width;
	textview->pos_x = textview->num_width;
	textview->pos_y = 0;

	return(0);
}

static int _textview_put_linenum(struct textview *textview)
{
	struct widget *widget;

	if(!textview) {
		return(-EINVAL);
	}

	widget = (struct widget*)textview;

	mvprintw(widget->y + textview->pos_y, widget->x, "%*d ",
		 textview->num_width - 1, textview->cur_line);
	mvchgat(widget->y + textview->pos_y, widget->x,
		textview->num_width, 0, UI_COLOR_LINES, NULL);

	return(0);
}

static int _textview_putc(struct textview *textview, const char chr)
{
	struct widget *widget;

	if(!textview) {
		return(-EINVAL);
	}

	widget = (struct widget*)textview;

	if(textview->pos_x > widget->width) {
		textview->pos_x = textview->num_width;
		textview->pos_y++;
	}

	if(textview->pos_x == textview->num_width) {
	        _textview_put_linenum(textview);
	}

	if(chr == '\n') {
		textview->pos_x = textview->num_width;
		textview->pos_y++;
		textview->cur_line++;
	} else {
		mvprintw(widget->y + textview->pos_y,
			 widget->x + textview->pos_x,
			 "%c", chr);
		mvchgat(widget->y + textview->pos_y,
			widget->x + textview->pos_x,
			1, 0, UI_COLOR_NORMAL, NULL);

		textview->pos_x++;
	}

	return(0);
}

static int _textview_puts(struct textview *textview, const char *str)
{
	int len;

	if(!textview) {
		return(-EINVAL);
	}

	for(len = 0; *str; len++, str++) {
		int err;

		err = 0;

	        if(*str == '\t') {
			int cur_pos;
			int cols_since_tabstop;
			int cols_to_tabstop;

			cur_pos = textview->pos_x - textview->num_width;
			cols_since_tabstop = cur_pos % textview->tab_width;
			cols_to_tabstop = textview->tab_width - cols_since_tabstop;

			while(cols_to_tabstop-- > 0) {
				err = _textview_putc(textview, ' ');

				if(err < 0) {
					break;
				}
			}
		} else {
			err = _textview_putc(textview, *str);
		}

		if(err < 0) {
			break;
		}
	}

	return(len);
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

static int _textview_draw_snippet(struct textview *textview, struct snippet *snippet)
{
	struct line *line;

	if(!textview || !snippet) {
		return(-EINVAL);
	}

	line = snippet_get_first_line(snippet);

	if(!line) {
		return(-EINVAL);
	}

	_textview_reset(textview, line_get_number(line));

	for( ; line; line = line_get_next(line)) {
		_textview_puts(textview, line_get_data(line));
	}

	return(0);
}

static int _textview_redraw(struct widget *widget)
{
	struct textview *textview;
	struct snippet *snip;
	int max_lines;
	int err;

	if(!widget) {
		return(-EINVAL);
	}

	textview = (struct textview*)widget;
	max_lines = widget->height - 1;

	widget_clear(widget, 0, 0, widget->width, widget->height);

	if(textview->start) {
		err = buffer_get_snippet_telex(textview->buffer,
					       textview->start,
					       textview->end,
					       max_lines,
					       &snip);
	} else {
		err = buffer_get_snippet(textview->buffer, 1, max_lines, &snip);
	}

	if(!err) {
		err = _textview_draw_snippet(textview, snip);
		snippet_free(&snip);
	}

	_textview_draw_status(textview);

	return(err);
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

	view->tab_width = config.tab_width;
	view->pos_x = 0;
	view->pos_y = 0;

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

	selection_changed = FALSE;

	if(textview->start != start || textview->end != end) {
		selection_changed = TRUE;
	}

	textview->start = start;
	textview->end = end;

	if(selection_changed) {
		widget_redraw((struct widget*)textview);
	}

	return(0);
}

int textview_set_selection_start(struct textview *textview, struct telex *start)
{
	int selection_changed;

	if(!textview) {
		return(-EINVAL);
	}

	selection_changed = textview->start != start;
	textview->start = start;

	if(selection_changed) {
		widget_redraw((struct widget*)textview);
	}

	return(0);
}

int textview_set_selection_end(struct textview *textview, struct telex *end)
{
	int selection_changed;

	if(!textview) {
		return(-EINVAL);
	}

	selection_changed = textview->end != end;
	textview->end = end;

	if(selection_changed) {
		widget_redraw((struct widget*)textview);
	}

	return(0);
}
