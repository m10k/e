#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"
#include "multistring.h"
#include "kbdwidget.h"

struct pos {
	int x;
	int y;
};

struct cmdbox {
	struct kbd_widget _parent;
	struct multistring *buffer;

	struct pos cursor;
	struct pos viewport;

	int highlight_start;
	int highlight_len;
	ui_color_t highlight_color;
};

static int _box_insert_at_cursor(struct cmdbox *box, const char chr)
{
	int err;

	if (!box) {
		return -EINVAL;
	}

	if ((err = multistring_line_insert_char(box->buffer, box->cursor.y, box->cursor.x, chr)) < 0) {
		return err;
	}

	box->cursor.x++;
	if (chr == '\n') {
		box->cursor.y++;
		box->cursor.x = 0;
	}

	return 0;
}

static int _box_remove_cursor_left(struct cmdbox *box)
{
	if (!box) {
		return -EINVAL;
	}

	if (box->cursor.x == 0 && box->cursor.y > 0) {
		box->cursor.y--;
		box->cursor.x = multistring_line_get_length(box->buffer, box->cursor.y);
	}

	if (box->cursor.x > 0) {
		multistring_line_remove_char(box->buffer, box->cursor.y, --box->cursor.x);
	}

	return 0;
}

static int _box_remove_cursor_right(struct cmdbox *box)
{
	if (!box) {
		return -EINVAL;
	}

	if (box->cursor.x < multistring_line_get_length(box->buffer, box->cursor.y)) {
		multistring_line_remove_char(box->buffer, box->cursor.y, box->cursor.x);
	}

	return 0;
}

static int _box_clear_input(struct cmdbox *box)
{
	if (!box) {
		return -EINVAL;
	}

	multistring_truncate(box->buffer, 0, 0);
	box->cursor.x = 0;
	box->cursor.y = 0;
	box->viewport.x = 0;
	box->viewport.y = 0;

	return 0;
}

static int _box_move_cursor(struct cmdbox *box, const int rel)
{
	int new_pos;

	new_pos = box->cursor.x + rel;

	if (new_pos < 0 || new_pos > multistring_line_get_length(box->buffer, box->cursor.y)) {
		return -EOVERFLOW;
	}

	box->cursor.x = new_pos;
	return 0;
}

static int _box_set_cursor(struct cmdbox *box, const int pos)
{
	int new_pos;
	int limit;

	if (!box) {
		return -EINVAL;
	}

	limit = multistring_line_get_length(box->buffer, box->cursor.y);

	if (pos < 0 || pos > limit) {
		new_pos = limit;
	} else {
		new_pos = pos;
	}

	box->cursor.x = new_pos;
	return 0;
}

static int _cmdbox_resize(struct widget *widget)
{
	if (!widget) {
		return -EINVAL;
	}

	widget->height = 2;
	return 0;
}

static int _cmdbox_blank(struct widget *widget)
{
	int y;

	if (!widget) {
		return -EINVAL;
	}

	for (y = 0; y < widget->height; y++) {
		int x;

		for (x = 0; x < widget->width; x++) {
			mvwaddch(widget->window,
				 widget->y + y, widget->x + x,
				 ' ');
		}

		mvchgat(widget->y + y, widget->x, widget->width,
			0, UI_COLOR_COMMAND, NULL);
	}

	return 0;
}

static int _cmdbox_redraw(struct widget *widget)
{
	struct cmdbox *box;
	int dist;
	int num_lines;
	int y;

	if (!widget) {
		return -EINVAL;
	}

	box = (struct cmdbox*)widget;

	_cmdbox_blank(widget);

	/* Slide the viewport if the cursor is out of view */
	if ((dist = box->cursor.x - box->viewport.x) < 0 ||
	    (dist = box->cursor.x - box->viewport.x - widget->width + 1) > 0) {
		box->viewport.x += dist;
	}
	if ((dist = box->cursor.y - box->viewport.y) < 0 ||
	    (dist = box->cursor.y - box->viewport.y - widget->height + 1) > 0) {
		box->viewport.y += dist;
	}

	fprintf(stderr, "Viewport @ (%d,%d)\n", box->viewport.x, box->viewport.y);
	num_lines = multistring_get_lines(box->buffer);

	fprintf(stderr, "for (y = 0; y < %d && box->viewport.y + y < %d; y++) { ... }\n",
		widget->height, num_lines);

	for (y = 0; y < widget->height && box->viewport.y + y < num_lines; y++) {
		const char *line_data;

		line_data = multistring_line_get_data(box->buffer, box->viewport.y + y);
		mvwprintw(widget->window, widget->y + y, widget->x,
			  "%.*s", widget->width, line_data + box->viewport.x);
		fprintf(stderr, "mvwprintw(%p, %d, %d, \"%%.*s\", %d, \"%s\");\n", (void*)widget->window, widget->y + y, widget->x, widget->width,
			line_data + box->viewport.x);
	}

	if(box->highlight_len > 0) {
		int highlight_x;
		int highlight_w;

		highlight_x = box->highlight_start - box->viewport.x;
		highlight_w = box->highlight_len - box->viewport.x;

		if (highlight_x < 0) {
			highlight_x = 0;
		}
		if (highlight_w < 0) {
			highlight_w = 0;
		}
		if (highlight_w > widget->width) {
			highlight_w = widget->width;
		}

		widget_set_color(widget, UI_COLOR_COMMAND,
				 0, 0, -1, -1);
		if (highlight_x < widget->width) {
			widget_set_color(widget, box->highlight_color,
					 highlight_x, 0,
					 highlight_w, 1);
		}
	}

	move(widget->y + box->cursor.y - box->viewport.y, widget->x + box->cursor.x - box->viewport.x);

	return(0);
}

static int _cmdbox_free(struct widget *widget)
{
	struct cmdbox *box;

	if(!widget) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;

	multistring_free(&(box->buffer));
	memset(box, 0, sizeof(*box));
	free(box);

	return(0);
}

static int _cmdbox_key_pressed(struct widget *widget, void *user_data, void *event)
{
	struct cmdbox *box;
	struct key_event *key_event;
	int err;

	box = (struct cmdbox*)widget;
	key_event = (struct key_event*)event;

	fprintf(stderr, "%s: %d + %d\n", __func__, key_event->keycode, key_event->modifier);

	if (!key_event->modifier) {
		switch (key_event->keycode) {
		case KEYCODE_DELETE:
			_box_remove_cursor_right(box);
			break;

		case KEYCODE_BACKSPACE:
			_box_remove_cursor_left(box);
			break;

		case KEYCODE_LEFT:
			_box_move_cursor(box, -1);
			break;

		case KEYCODE_RIGHT:
			_box_move_cursor(box, +1);
			break;

		case KEYCODE_DOWN:
		case KEYCODE_END:
			_box_set_cursor(box, -1);
			break;

		case KEYCODE_UP:
		case KEYCODE_HOME:
			_box_set_cursor(box, 0);
			break;

		case KEYCODE_F1:
		case KEYCODE_F2:
		case KEYCODE_F3:
		case KEYCODE_F4:
		case KEYCODE_F5:
		case KEYCODE_F6:
		case KEYCODE_F7:
		case KEYCODE_F8:
		case KEYCODE_F9:
		case KEYCODE_F10:
		case KEYCODE_F11:
		case KEYCODE_F12:
			break;

		default:
			if ((err = _box_insert_at_cursor(box, key_event->keycode)) < 0) {
				fprintf(stderr, "_box_insert_at_cursor: %s\n", strerror(-err));
			}
		}
	} else if (key_event->modifier & KEYMOD_CTRL) {
		switch (key_event->keycode) {
		case 'Q':
			widget_emit_signal(widget, "quit_requested", NULL);
			break;

		case 'A':
			_box_set_cursor(box, 0);
			break;

		case 'E':
			_box_set_cursor(box, -1);
			break;

		case 'B':
			_box_move_cursor(box, -1);
			break;

		case 'F':
			_box_move_cursor(box, +1);
			break;

		case 'S':
			widget_emit_signal(widget, "save_requested", box->buffer);
			break;

		case 'R':
			widget_emit_signal(widget, "erase_requested", box->buffer);
			break;

		case 'D':
			_box_remove_cursor_right(box);
			break;

		case 'Z':
			widget_emit_signal(widget, "selection_start_changed", box->buffer);
			break;

		case 'X':
			widget_emit_signal(widget, "selection_end_changed", box->buffer);
			break;

		case 'C':
			widget_emit_signal(widget, "read_requested", box->buffer);
			break;

		case 'V':
			widget_emit_signal(widget, "write_requested", box->buffer);
			break;

		case 'H':
			_box_remove_cursor_left(box);
			break;

		case 'J':
			_box_insert_at_cursor(box, '\n');
			break;

		case 'P':
			widget_emit_signal(widget, "insert_requested", box->buffer);
			break;

		case 'N':
			widget_emit_signal(widget, "oinsert_requested", box->buffer);
			break;
		}
	}

	return widget_redraw(widget);
}

int cmdbox_new(struct cmdbox **cmdbox)
{
	struct cmdbox *box;

	if(!cmdbox) {
		return(-EINVAL);
	}

	box = malloc(sizeof(*box));

	if(!box) {
		return(-ENOMEM);
	}

	memset(box, 0, sizeof(*box));

	kbd_widget_init((struct kbd_widget*)box);

	/* don't vertically expand cmdboxes */
	((struct widget*)box)->attrs &= ~UI_ATTR_VEXPAND;

	if (multistring_new(&(box->buffer)) < 0) {
		free(box);
		return(-ENOMEM);
	}

	((struct widget*)box)->resize = _cmdbox_resize;
	((struct widget*)box)->redraw = _cmdbox_redraw;
	((struct widget*)box)->free = _cmdbox_free;

	widget_add_handler((struct widget*)box, "key_pressed", _cmdbox_key_pressed, NULL);

	widget_add_signal((struct widget*)box, "selection_start_changed");
	widget_add_signal((struct widget*)box, "selection_end_changed");
	widget_add_signal((struct widget*)box, "read_requested");
	widget_add_signal((struct widget*)box, "write_requested");
	widget_add_signal((struct widget*)box, "insert_requested");
	widget_add_signal((struct widget*)box, "oinsert_requested");
	widget_add_signal((struct widget*)box, "save_requested");
	widget_add_signal((struct widget*)box, "erase_requested");
	widget_add_signal((struct widget*)box, "quit_requested");

	*cmdbox = box;

	return(0);
}

int cmdbox_highlight(struct cmdbox *box, const ui_color_t color,
		     const int pos, const int len)
{
	ui_color_t eff_color;
	int eff_len;
	int width;

	if(!box) {
		return(-EINVAL);
	}

	width = ((struct widget*)box)->width;

	if(pos < 0 || pos > width) {
		return(-EOVERFLOW);
	}

	if(len < 0 || pos + len > width) {
		eff_len = width - pos;
	} else {
		eff_len = len;
	}

	if(color < UI_COLOR_DEFAULT || color > UI_COLOR_COMMAND) {
		eff_color = UI_COLOR_COMMAND;
	} else {
		eff_color = color;
	}

	box->highlight_color = eff_color;
	box->highlight_start = pos;
	box->highlight_len = eff_len;

	return(widget_redraw((struct widget*)box));
}

int cmdbox_clear(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	_box_clear_input(box);
	cmdbox_highlight(box, UI_COLOR_DELETION, 0, 0);

	return(0);
}

int cmdbox_set_text(struct cmdbox *box, const char *text)
{
	const char *pos;

	if(!box || !text) {
		return(-EINVAL);
	}

	cmdbox_clear(box);

	for(pos = text; *pos; pos++) {
		_box_insert_at_cursor(box, *pos);
	}

	widget_redraw((struct widget*)box);

	return(0);
}

char* cmdbox_get_text(struct cmdbox *box)
{
	if(!box) {
		return(NULL);
	}

	return (multistring_get_data(box->buffer));
}

int cmdbox_get_length(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	return(multistring_get_length(box->buffer));
}
