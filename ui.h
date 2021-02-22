#ifndef E_UI_H
#define E_UI_H

#include <ncurses.h>
#include "buffer.h"

typedef enum {
	UI_COLOR_DEFAULT = 0,
	UI_COLOR_NORMAL,
	UI_COLOR_LINES,
	UI_COLOR_SELECTION,
	UI_COLOR_DELETION,
	UI_COLOR_INSERTION,
	UI_COLOR_STATUS
} ui_color_t;

typedef enum {
	UI_ATTR_HEXPAND = (1 << 0),
	UI_ATTR_VEXPAND = (1 << 1),
	UI_ATTR_EXPAND  = (UI_ATTR_HEXPAND | UI_ATTR_VEXPAND)
} ui_attr_t;

struct widget {
	int x;
	int y;
	int width;
	int height;
	int color;
	ui_attr_t attrs;

	WINDOW *window;
	struct widget *parent;

	int (*input)(struct widget*, const int);
	int (*resize)(struct widget*);
	int (*redraw)(struct widget*);
	int (*free)(struct widget*);
};

struct container {
	struct widget _parent;

	int (*add)(struct container*, struct widget*);
};

struct window;
struct cmdbox;
struct vbox;
struct textview;

#define widget_input(w,c) ((w)->input((w), (c)))
#define widget_resize(w)  ((w)->resize((w)))
#define widget_redraw(w)  ((w)->redraw((w)))
#define widget_free(w)    ((w)->free((w)))
#define widget_set_size(widget,w,h) do {	\
		(widget)->width = (w);	\
		(widget)->height = (h);	\
	} while(0);
#define widget_set_position(widget,_x,_y) do {	\
		(widget)->x = (_x);		\
		(widget)->y = (_y);		\
	} while(0);


int widget_clear(struct widget *widget, const int x, const int y,
		 const int width, const int height);

#define container_add(c,w) ((c)->add((c), (w)))

int window_new(struct window **window);
int cmdbox_new(struct cmdbox **cmdbox);
int vbox_new(struct vbox **vbox, int size);
int textview_new(struct textview **textview);

int textview_set_buffer(struct textview *textview, struct buffer *buffer);
int textview_set_selection(struct textview *textview, struct telex *start, struct telex *end);

#endif /* E_UI_H */
