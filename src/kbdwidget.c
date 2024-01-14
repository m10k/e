#include <stdlib.h>
#include <errno.h>
#include "ui.h"
#include "kbdwidget.h"

struct keymap {
	struct key_event key;
	int (*next_handler)(struct kbd_widget*, const int);
};

static const char *_signals[] = {
	"key_pressed"
};

static int _kbd_handler_single(struct kbd_widget *kbd, const int event);
static int _kbd_handler_double(struct kbd_widget *kbd, const int event);
static int _kbd_handler_triple(struct kbd_widget *kbd, const int event);
static int _kbd_handler_pgup(struct kbd_widget *kbd, const int event);
static int _kbd_handler_pgdn(struct kbd_widget *kbd, const int event);
static int _kbd_handler_ins(struct kbd_widget *kbd, const int event);
static int _kbd_handler_del(struct kbd_widget *kbd, const int event);
static int _kbd_handler_fn(struct kbd_widget *kbd, const int event);

static int _kbd_handler_single(struct kbd_widget *kbd, const int event)
{
	int (*next_handler)(struct kbd_widget*, const int);
	int err;
	struct key_event key;

	key.keycode = -1;
	key.modifier = KEYMOD_NONE;
	next_handler = _kbd_handler_single;
	err = 0;

	switch (event) {
	case 0: /* ^@ */
	case 1: /* ^A */
	case 2: /* ^B */
	case 3: /* ^C */
	case 4: /* ^D */
	case 5: /* ^E */
	case 6: /* ^F */
	case 7: /* ^G */
	case 8: /* ^H */
	case 9: /* ^I */
	case 10: /* ^J */
	case 11: /* ^K */
	case 12: /* ^L */
	case 13: /* ^M */
	case 14: /* ^N */
	case 15: /* ^O */
	case 16: /* ^P */
	case 17: /* ^Q */
	case 18: /* ^R */
	case 19: /* ^S */
	case 20: /* ^T */
	case 21: /* ^U */
	case 22: /* ^V */
	case 23: /* ^W */
	case 24: /* ^X */
	case 25: /* ^Y */
	case 26: /* ^Z */
	case 28: /* ^\ */
	case 29: /* ^] */
	case 30: /* ^^ */
	case 31: /* ^_ */
		key.keycode = '@' + event;
		key.modifier = KEYMOD_CTRL;
		break;

	case 27: /* ^[ */
		next_handler = _kbd_handler_double;
		break;

	default:
		key.keycode = event;
		break;
	}

	if (key.keycode >= 0) {
		err = widget_emit_signal((struct widget*)kbd, "key_pressed", &key);
	}

	kbd->_key_handler = next_handler;

	return err;
}

static int _kbd_handler_double(struct kbd_widget *kbd, const int event)
{
	int (*next_handler)(struct kbd_widget*, const int);
	int err;

	next_handler = _kbd_handler_single;
	err = 0;

	switch (event) {
	case 27:
		err = -EIO;
		break;

	case 79:
		next_handler = _kbd_handler_fn;
		break;

	case 91:
		next_handler = _kbd_handler_triple;
		break;

	default:
		break;
	}

	kbd->_key_handler = next_handler;

	return err;
}

static int _kbd_handler_triple(struct kbd_widget *kbd, const int event)
{
	int (*next_handler)(struct kbd_widget*, const int);
	struct key_event key;
	int err;

	next_handler = _kbd_handler_single;
	key.keycode = -1;
	key.modifier = KEYMOD_NONE;
	err = 0;

	switch (event) {
	case 65: /* up */
	case 66: /* down */
	case 67: /* right */
	case 68: /* left */
	case 70: /* end */
	case 72: /* home */
		key.keycode = KEYCODE_UP + event - 65;
		break;

	case 53: /* PgUp */
		next_handler = _kbd_handler_pgup;
		break;

	case 54: /* PgDn */
		next_handler = _kbd_handler_pgdn;
		break;

	case 50: /* Ins */
		next_handler = _kbd_handler_ins;
		break;

	case 51: /* Del */
		next_handler = _kbd_handler_del;
		break;

	default:
		break;
	}

	if (key.keycode >= 0) {
		err = widget_emit_signal((struct widget*)kbd, "key_pressed", &key);
	}

	kbd->_key_handler = next_handler;

	return err;
}

static int _kbd_handler_pgup(struct kbd_widget *kbd, const int event)
{
	kbd->_key_handler = _kbd_handler_single;

	return event == 126 ? 0 : -EINVAL;
}

static int _kbd_handler_pgdn(struct kbd_widget *kbd, const int event)
{
	kbd->_key_handler = _kbd_handler_single;

	return event == 126 ? 0 : -EINVAL;
}

static int _kbd_handler_ins(struct kbd_widget *kbd, const int event)
{
	kbd->_key_handler = _kbd_handler_single;

	return event == 126 ? 0 : -EINVAL;
}

static int _kbd_handler_del(struct kbd_widget *kbd, const int event)
{
	struct key_event key;
	int err;

	if (event == 126) {
		key.keycode = KEYCODE_DELETE;
		key.modifier = KEYMOD_NONE;

		err = widget_emit_signal((struct widget*)kbd, "key_pressed", &key);
	} else {
		err = -EINVAL;
	}

	kbd->_key_handler = _kbd_handler_single;
	return err;
}

static int _kbd_handler_fn(struct kbd_widget *kbd, const int event)
{
	struct key_event key;
	int err;

	switch (event) {
	case 80: /* F1 */
	case 81: /* F2 */
	case 82: /* F3 */
	case 83: /* F4 */
	case 84: /* F5 */
		key.keycode = KEYCODE_F1 + event - 80;
		key.modifier = KEYMOD_NONE;

		err = widget_emit_signal((struct widget*)kbd, "key_pressed", &key);
		break;

	default:
		err = -EINVAL;
		break;
	}

	kbd->_key_handler = _kbd_handler_single;

	return err;
}

static int _kbd_widget_input(struct kbd_widget *kbd, const int event)
{
	if (!kbd) {
		return -EINVAL;
	}

	return kbd->_key_handler(kbd, event);
}

int kbd_widget_init(struct kbd_widget *kbd)
{
	int err;
	int i;

	if ((err = widget_init((struct widget*)kbd)) < 0) {
		return err;
	}

	kbd->_key_handler = _kbd_handler_single;
	((struct widget*)kbd)->input = (int(*)(struct widget*, const int))_kbd_widget_input;

	for (i = 0; i < (sizeof(_signals) / sizeof(*_signals)); i++) {
		err = widget_add_signal((struct widget*)kbd, _signals[i]);

		if (err < 0) {
			return err;
		}
	}

	return err;
}
