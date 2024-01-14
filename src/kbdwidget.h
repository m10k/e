#ifndef E_KBDWIDGET_H
#define E_KBDWIDGET_H

#include "ui.h"

typedef enum {
	KEYCODE_DELETE    = 126,
	KEYCODE_BACKSPACE = 127,
	KEYCODE_UP        = 255,
	KEYCODE_DOWN      = 256,
	KEYCODE_RIGHT     = 257,
	KEYCODE_LEFT      = 258,
	KEYCODE_END       = 260,
	KEYCODE_HOME      = 262,
	KEYCODE_F1        = 300,
	KEYCODE_F2        = 301,
	KEYCODE_F3        = 302,
	KEYCODE_F4        = 303,
	KEYCODE_F5        = 304,
	KEYCODE_F6        = 305,
	KEYCODE_F7        = 306,
	KEYCODE_F8        = 307,
	KEYCODE_F9        = 308,
	KEYCODE_F10       = 309,
	KEYCODE_F11       = 310,
	KEYCODE_F12       = 311
} key_code_t;

typedef enum {
	KEYMOD_NONE  = 0,
	KEYMOD_LCTRL = (1 << 0),
	KEYMOD_RCTRL = (1 << 1),
	KEYMOD_CTRL  = (KEYMOD_LCTRL | KEYMOD_RCTRL),
	KEYMOD_LALT  = (1 << 2),
	KEYMOD_RALT  = (1 << 3),
	KEYMOD_ALT   = (KEYMOD_LALT | KEYMOD_RALT),
	KEYMOD_SUPER = (1 << 4)
} key_mod_t;

struct key_event {
        int keycode;
        int modifier;
};

struct kbd_widget {
	struct widget _parent;

	int (*_key_handler)(struct kbd_widget*, const int);
};

int kbd_widget_init(struct kbd_widget *kbd);

#endif /* E_KBDWIDGET_H */
