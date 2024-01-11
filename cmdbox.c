#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"
#include "string.h"
#include "kbdwidget.h"

struct cmdbox {
	struct kbd_widget _parent;
	struct string *buffer;

	struct {
		int x;
		int y;
	} cursor;
	int max_length;

	int highlight_start;
	int highlight_len;
	ui_color_t highlight_color;
};

static int _box_insert_at_cursor(struct cmdbox *box, const char chr)
{
	if(!box) {
		return(-EINVAL);
	} else if(string_get_length(box->buffer) == box->max_length) {
		return(-EOVERFLOW);
	} else if(string_insert_char(box->buffer, box->cursor.x, chr) < 0) {
		return(-ENOMEM);
	} else {
		box->cursor.x++;
	}

	return(0);
}

static int _box_remove_cursor_left(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	if(box->cursor.x > 0) {
		string_remove_char(box->buffer, --box->cursor.x);
	}

	return(0);
}

static int _box_remove_cursor_right(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	if(box->cursor.x < string_get_length(box->buffer)) {
		string_remove_char(box->buffer, box->cursor.x);
	}

	return(0);
}

static int _box_clear_input(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	string_truncate(box->buffer, 0);
	box->cursor.x = 0;

	return(0);
}

static int _box_move_cursor(struct cmdbox *box, const int rel)
{
	int new_pos;

	new_pos = box->cursor.x + rel;

	if(new_pos < 0 || new_pos > string_get_length(box->buffer)) {
		return(-EOVERFLOW);
	}

	box->cursor.x = new_pos;
	return(0);
}

static int _box_set_cursor(struct cmdbox *box, const int pos)
{
	int new_pos;
	int limit;

	if(!box) {
		return(-EINVAL);
	}

	limit = string_get_length(box->buffer);

	if(pos < 0 || pos > limit) {
		new_pos = limit;
	} else {
		new_pos = pos;
	}

	box->cursor.x = new_pos;
	return(0);
}

static int _cmdbox_resize(struct widget *widget)
{
	struct cmdbox *box;

	if(!widget) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;

	widget->height = 1;
	box->max_length = widget->width;

	return(0);
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

	if(!widget) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;

	_cmdbox_blank(widget);

	mvwprintw(widget->window, widget->y, widget->x,
		  "%s", string_get_data(box->buffer));

	if(box->highlight_len > 0) {
		widget_set_color(widget, UI_COLOR_COMMAND,
				 0, 0, -1, -1);
		widget_set_color(widget, box->highlight_color,
				 box->highlight_start, 0,
				 box->highlight_len, 1);
	}

	move(widget->y + box->cursor.y, widget->x + box->cursor.x);

	return(0);
}

static int _cmdbox_free(struct widget *widget)
{
	struct cmdbox *box;

	if(!widget) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;

	string_free(&(box->buffer));
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

	if(string_new(&(box->buffer)) < 0) {
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

const char* cmdbox_get_text(struct cmdbox *box)
{
	if(!box) {
		return(NULL);
	}

	return(string_get_data(box->buffer));
}

int cmdbox_get_length(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	return(string_get_length(box->buffer));
}
