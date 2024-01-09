#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ui.h"
#include "string.h"

typedef int (_key_handler)(struct cmdbox*, const int);

static int _key_handler_single(struct cmdbox*, const int);
static int _key_handler_double(struct cmdbox*, const int);
static int _key_handler_triple(struct cmdbox*, const int);

static _key_handler *_handler = _key_handler_single;

struct cmdbox {
	struct widget _parent;
	struct string *buffer;

	int cursor;
	int max_length;

	int highlight_start;
	int highlight_len;
	ui_color_t highlight_color;
};

#if 0
static int _box_insert_at_pos(struct cmdbox *box, int pos, const char chr)
{
	if(!box) {
		return(-EINVAL);
	} else if(string_get_length(box->buffer) == box->max_length) {
		return(-EOVERFLOW);
	} else if(string_insert_char(box->buffer, pos, chr) < 0) {
		return(-ENOMEM);
	}

	return(0);
}
#endif /* 0 */

static int _box_insert_at_cursor(struct cmdbox *box, const char chr)
{
	if(!box) {
		return(-EINVAL);
	} else if(string_get_length(box->buffer) == box->max_length) {
		return(-EOVERFLOW);
	} else if(string_insert_char(box->buffer, box->cursor, chr) < 0) {
		return(-ENOMEM);
	} else {
		box->cursor++;
	}

	return(0);
}

static int _box_remove_cursor_left(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	if(box->cursor > 0) {
		string_remove_char(box->buffer, --box->cursor);
	}

	return(0);
}

static int _box_remove_cursor_right(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	if(box->cursor < string_get_length(box->buffer)) {
		string_remove_char(box->buffer, box->cursor);
	}

	return(0);
}

static int _box_clear_input(struct cmdbox *box)
{
	if(!box) {
		return(-EINVAL);
	}

	string_truncate(box->buffer, 0);
	box->cursor = 0;

	return(0);
}

static int _box_move_cursor(struct cmdbox *box, const int rel)
{
	int new_pos;

	new_pos = box->cursor + rel;

	if(new_pos < 0 || new_pos > string_get_length(box->buffer)) {
		return(-EOVERFLOW);
	}

	box->cursor = new_pos;
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

	box->cursor = new_pos;
	return(0);
}

static int _key_handler_single(struct cmdbox *box, const int key)
{
	_key_handler *next_handler;
	int err;

	next_handler = _key_handler_single;
	err = 0;

#ifdef DEBUG_INPUT
	fprintf(stderr, "S[%d]\n", key);
#endif /* DEBUG_INPUT */

	switch(key) {
#if 0
	case 0: /* ^2, ^@ */
	case 17: /* ^Q */
	case 23: /* ^W */
	case 5   /* ^E */
	case 18: /* ^R */
	case 20: /* ^T */
	case 25: /* ^Y */
	case 21: /* ^U */
	case 9:  /* ^I */
	case 15: /* ^O */
	case 16: /* ^P */
	case 27: /* ESC, ^[ */
		break;

	case 1:  /* ^A */
	case 19: /* ^S */
	case 4:  /* ^D */
	case 6:  /* ^F */
	case 7:  /* ^G */
	case 8:  /* ^H */
	case 10: /* ^J */
	case 11: /* ^K */
	case 12: /* ^L */
	case 59: /* ; */
	case 58: /* : */
	case 29: /* ^] */
		break;

	case 26:  /* ^Z */
	case 24:  /* ^X */
	case 3:   /* ^C */
	case 22:  /* ^V */
	case 2:   /* ^B */
	case 14:  /* ^N */
	case 13:  /* ^M (usually mapped to 10: ^J) */
	case 44:  /* , */
	case 46:  /* . */
	case 31:  /* ^_ */
	case 28:  /* ^\ */
	case 127: /* ^? */
	case 30:  /* ^^ */

#endif
	case 2:   /* ^B */
		_box_move_cursor(box, -1);
		break;

	case 6:   /* ^F */
		_box_move_cursor(box, +1);
		break;

	case 5:   /* ^E */
		_box_set_cursor(box, -1);
		break;

	case 1:   /* ^A */
		_box_set_cursor(box, 0);
		break;

	case 19:  /* ^S */
		widget_emit_signal((struct widget*)box,
				   "save_requested",
				   box->buffer);
		break;

	case 18:  /* ^R */
		widget_emit_signal((struct widget*)box,
				   "erase_requested",
				   box->buffer);
		break;

	case 4:   /* ^D */
		_box_remove_cursor_right(box);
		break;

	case 26:  /* ^Z */
		widget_emit_signal((struct widget*)box,
				   "selection_start_changed",
				   box->buffer);
		break;

	case 24:  /* ^X */
		widget_emit_signal((struct widget*)box,
				   "selection_end_changed",
				   box->buffer);
		break;

	case 3:   /* ^C */
		widget_emit_signal((struct widget*)box,
				   "read_requested",
				   box->buffer);
		break;

	case 22:  /* ^V */
		widget_emit_signal((struct widget*)box,
				   "write_requested",
				   box->buffer);
		break;

	case 8:   /* ^H */
	case 127: /* backspace */
		_box_remove_cursor_left(box);
		break;

	case 10:
		/* enter */
		widget_emit_signal((struct widget*)box,
				   "insert_requested",
				   box->buffer);
		break;

	case 14:  /* ^N */
		widget_emit_signal((struct widget*)box,
				   "oinsert_requested",
				   box->buffer);
		break;

	case 27:
		next_handler = _key_handler_double;
		break;

	default:
		_box_insert_at_cursor(box, (char)key);
		break;
	}

	_handler = next_handler;
	return(err);
}

static int _key_handler_fn(struct cmdbox *box, const int key)
{
	int err;

	err = 0;

	switch(key) {
	case 80: /* F1 */
		break;

	case 81: /* F2 */
		break;

	case 82: /* F3 */
		break;

	case 83: /* F4 */
		break;

	case 84: /* F5 */
		break;

	default:
		err = -EINVAL;
		break;
	}

	_handler = _key_handler_single;
	return(err);
}

static int _key_handler_double(struct cmdbox *box, const int key)
{
	_key_handler *next_handler;
	int err;

	next_handler = _key_handler_single;
	err = 0;

#ifdef DEBUG_INPUT
	fprintf(stderr, "D[%d]\n", key);
#endif /* DEBUG_INPUT */

	switch(key) {
	case 27:
		next_handler = _key_handler_single;
		err = -EIO;
		break;

	case 79:
		next_handler = _key_handler_fn;
		break;

	case 91:
		next_handler = _key_handler_triple;
		break;

	default:
		break;
	}

	_handler = next_handler;
	return(err);
}

static int _key_handler_pgup(struct cmdbox *box, const int key)
{
	int err;

	if(key == 126) {
		err = 0;
	} else {
		err = -EINVAL;
	}

	_handler = _key_handler_single;
	return(err);
}

static int _key_handler_pgdn(struct cmdbox *box, const int key)
{
	int err;

	if(key == 126) {
		err = 0;
	} else {
		err = -EINVAL;
	}

	_handler = _key_handler_single;
	return(err);
}

static int _key_handler_ins(struct cmdbox *box, const int key)
{
	int err;

        if(key == 126) {
		err = 0;
	} else {
		err = -EINVAL;
	}

	_handler = _key_handler_single;
	return(err);
}

static int _key_handler_del(struct cmdbox *box, const int key)
{
	int err;

	err = 0;

	if(key == 126) {
		_box_remove_cursor_right(box);
	} else {
		err = -EINVAL;
	}

	_handler = _key_handler_single;
	return(err);
}

static int _key_handler_triple(struct cmdbox *box, const int key)
{
	_key_handler *next_handler;
	int err;

	next_handler = _key_handler_single;
	err = 0;

#if DEBUG_INPUT
	fprintf(stderr, "T[%d]\n", key);
#endif /* DEBUG_INPUT */

	switch(key) {
	case 68:
		/* left */
		_box_move_cursor(box, -1);
		break;

	case 67:
		/* right */
		_box_move_cursor(box, +1);
		break;

	case 65: /* up */
		break;

	case 66: /* dn */
		break;

	case 72: /* home */
		box->cursor = 0;
		break;

	case 70: /* end */
		box->cursor = string_get_length(box->buffer);
		break;

	case 53: /* PgUp */
		next_handler = _key_handler_pgup;
		break;

	case 54: /* PgDn */
		next_handler = _key_handler_pgdn;
		break;

	case 50: /* Ins */
		next_handler = _key_handler_ins;
		break;

	case 51: /* Del */
		next_handler = _key_handler_del;
		break;

	default:
		break;
	}

	_handler = next_handler;
	return(err);
}

static int _cmdbox_input(struct widget *widget, const int ev)
{
	struct cmdbox *box;
	int err;

	if(!widget) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;

	err = _handler(box, ev);

	if(err < 0) {
		return(err);
	}

	return(widget_redraw(widget));
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

	move(widget->y, widget->x + box->cursor);

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

	widget_init((struct widget*)box);

	/* don't vertically expand cmdboxes */
	((struct widget*)box)->attrs &= ~UI_ATTR_VEXPAND;

	if(string_new(&(box->buffer)) < 0) {
		free(box);
		return(-ENOMEM);
	}

	((struct widget*)box)->input = _cmdbox_input;
	((struct widget*)box)->resize = _cmdbox_resize;
	((struct widget*)box)->redraw = _cmdbox_redraw;
	((struct widget*)box)->free = _cmdbox_free;

	widget_add_signal((struct widget*)box, "selection_start_changed");
	widget_add_signal((struct widget*)box, "selection_end_changed");
	widget_add_signal((struct widget*)box, "read_requested");
	widget_add_signal((struct widget*)box, "write_requested");
	widget_add_signal((struct widget*)box, "insert_requested");
	widget_add_signal((struct widget*)box, "oinsert_requested");
	widget_add_signal((struct widget*)box, "save_requested");
	widget_add_signal((struct widget*)box, "erase_requested");

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
