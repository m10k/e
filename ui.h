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
	UI_COLOR_STATUS,
	UI_COLOR_COMMAND
} ui_color_t;

typedef enum {
	UI_ATTR_HEXPAND = (1 << 0),
	UI_ATTR_VEXPAND = (1 << 1),
	UI_ATTR_EXPAND  = (UI_ATTR_HEXPAND | UI_ATTR_VEXPAND),
	UI_ATTR_VISIBLE = (1 << 2)
} ui_attr_t;

struct signal;

struct widget {
	int x;
	int y;
	int width;
	int height;
	int color;
	ui_attr_t attrs;

	WINDOW *window;
	struct widget *parent;
	struct signal *signals;

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

typedef int (widget_handler_t)(struct widget*, void*, void*);

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

int widget_init(struct widget *widget);

int widget_set_visible(struct widget *widget, int is_visible);
int widget_is_visible(struct widget *widget);

int widget_clear(struct widget *widget, const int x, const int y,
		 const int width, const int height);
int widget_set_color(struct widget *widget, const ui_color_t color,
		     const int x, const int y,
		     const int width, const int height);

int widget_add_signal(struct widget *widget, const char *name);
int widget_remove_signal(struct widget *widget, const char *name);

int widget_add_handler(struct widget *widget, const char *name,
		       widget_handler_t *handler, void *user_data);
int widget_remove_handler(struct widget *widget, const char *name, widget_handler_t *handler);

int widget_emit_signal(struct widget *widget, const char *name, void *data);

int container_init(struct container *container);
#define container_add(c,w) ((c)->add((c), (w)))

int window_new(struct window **window);
int cmdbox_new(struct cmdbox **cmdbox);
int cmdbox_highlight(struct cmdbox *box, const ui_color_t color,
		     const int pos, const int len);
int cmdbox_clear(struct cmdbox *box);

int vbox_new(struct vbox **vbox, int size);
int textview_new(struct textview **textview);

int textview_set_buffer(struct textview *textview, struct buffer *buffer);
int textview_set_selection(struct textview *textview, struct telex *start, struct telex *end);
int textview_set_selection_start(struct textview *textview, struct telex *start);
int textview_set_selection_end(struct textview *textview, struct telex *end);

#endif /* E_UI_H */
