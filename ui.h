#ifndef E_UI_H
#define E_UI_H

#include <ncurses.h>

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

#define container_add(c,w) ((c)->add((c), (w)))

int window_new(struct window **window);
int window_set_child(struct window *window, struct widget *child);

int cmdbox_new(struct cmdbox **cmdbox);
int vbox_new(struct vbox **vbox);
int textview_new(struct textview**);

#endif /* E_UI_H */
