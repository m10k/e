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

static int _box_move_cursor(struct cmdbox *box, int rel)
{
	int new_pos;

	new_pos = box->cursor + rel;

	if(new_pos < 0 || new_pos > string_get_length(box->buffer)) {
		return(-EOVERFLOW);
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
	case 10:  /* ^J */
	case 44:  /* , */
	case 46:  /* . */
	case 31:  /* ^_ */
	case 28:  /* ^\ */
	case 127: /* ^? */
	case 30:  /* ^^ */

#endif


	case 127:
		/* backspace */
		_box_remove_cursor_left(box);
		break;

	case 10:
		/* enter */
		_box_clear_input(box);
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

static int _cmdbox_redraw(struct widget *widget)
{
	struct cmdbox *box;
	int x;

	if(!widget) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;

	for(x = 0; x <= widget->width; x++) {
		mvwaddch(widget->window, widget->y, widget->x + x, ' ');
	}

	mvwprintw(widget->window, widget->y, widget->x, "%s", string_get_data(box->buffer));
	move(box->_parent.y, box->cursor);

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

	if(string_new(&(box->buffer)) < 0) {
		free(box);
		return(-ENOMEM);
	}

	((struct widget*)box)->input = _cmdbox_input;
	((struct widget*)box)->resize = _cmdbox_resize;
	((struct widget*)box)->redraw = _cmdbox_redraw;
	((struct widget*)box)->free = _cmdbox_free;

	*cmdbox = box;

	return(0);
}
