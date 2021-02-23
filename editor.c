#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "editor.h"
#include "buffer.h"
#include "ui.h"

struct editor {
	struct window *window;
	struct vbox *vbox;
	struct textview *preedit;
	struct textview *postedit;
	struct cmdbox *cmdbox;

	struct buffer *prebuffer;
	struct buffer *postbuffer;
};

static int _cmdbox_selection_start_change(struct widget *widget, void *user_data, void *data)
{
	struct textview *textview;
	struct string *expr;
	const char *err_ptr;
	struct telex *telex;
	int err;

	if(!widget || !user_data || !data) {
		return(-EINVAL);
	}

	textview = (struct textview*)user_data;
	expr = (struct string*)data;

	err = telex_parse(string_get_data(expr),
			  &telex, &err_ptr);

	if(!err) {
		textview_set_selection_start(textview, telex);
		widget_set_visible((struct widget*)textview, TRUE);
	}

	widget_resize(widget->parent);
	widget_redraw(widget->parent);

	return(0);
}

static int _cmdbox_selection_end_change(struct widget *widget, void *user_data, void *data)
{
	struct textview *textview;
	struct string *expr;
	const char *err_ptr;
	struct telex *telex;
	int err;

	if(!widget || !user_data || !data) {
		return(-EINVAL);
	}

	textview = (struct textview*)user_data;
	expr = (struct string*)data;

	err = telex_parse(string_get_data(expr),
			  &telex, &err_ptr);

	if(!err) {
		textview_set_selection_end(textview, telex);
		widget_set_visible((struct widget*)textview, TRUE);
	}

	widget_resize(widget->parent);
	widget_redraw(widget->parent);

	return(0);
}

static int _editor_init_ui(struct editor *editor)
{
	int err;

	if(!editor) {
		return(-EINVAL);
	} else if((err = window_new(&(editor->window))) < 0) {
		return(err);
	} else if((err = vbox_new(&(editor->vbox), 4)) < 0) {
		return(err);
	} else if((err = container_add((struct container*)editor->window,
				       (struct widget*)editor->vbox)) < 0) {
		return(err);
	} else if((err = textview_new(&(editor->preedit))) < 0) {
		return(err);
	} else if((err = container_add((struct container*)editor->vbox,
				       (struct widget*)editor->preedit)) < 0) {
		return(err);
	} else if((err = textview_new(&(editor->postedit))) < 0) {
		return(err);
	} else if((err = container_add((struct container*)editor->vbox,
				       (struct widget*)editor->postedit)) < 0) {
		return(err);
	} else if((err = cmdbox_new(&(editor->cmdbox))) < 0) {
		return(err);
	} else if((err = container_add((struct container*)editor->vbox,
				       (struct widget*)editor->cmdbox)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "source_start_changed",
					    _cmdbox_selection_start_change,
					    editor->preedit)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "source_end_changed",
					    _cmdbox_selection_end_change,
					    editor->preedit)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "destination_start_changed",
					    _cmdbox_selection_start_change,
					    editor->postedit)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "destination_end_changed",
					    _cmdbox_selection_end_change,
					    editor->postedit)) < 0) {
		return(err);
	}

	widget_set_visible((struct widget*)editor->postedit, FALSE);

	widget_resize((struct widget*)editor->window);
	widget_redraw((struct widget*)editor->window);

	return(0);
}

int editor_open(struct editor *editor, const char *path)
{
	int err;

	if(!editor || !path) {
		return(-EINVAL);
	}

	if(editor->prebuffer) {
		return(-EALREADY);
	}

	err = buffer_open(&(editor->prebuffer), path);

	if(err < 0) {
		return(err);
	}

	err = textview_set_buffer(editor->preedit, editor->prebuffer);

	if(err < 0) {
		return(err);
	}

	err = buffer_clone(editor->prebuffer, &(editor->postbuffer));

	if(err < 0) {
		return(err);
	}

	err = textview_set_buffer(editor->postedit, editor->postbuffer);

	if(err < 0) {
		return(err);
	}

	widget_redraw((struct widget*)editor->window);

	return(0);
}

int editor_run(struct editor *editor)
{
	int event;

	while(1) {
		event = getch();

		if(event == 17) {
			break;
		}

		widget_input((struct widget*)editor->window, event);
	}

	return(0);
}

int editor_new(struct editor **editor)
{
	struct editor *edit;
	int err;

	if(!editor) {
		return(-EINVAL);
	}

	edit = malloc(sizeof(*edit));

	if(!edit) {
		return(-ENOMEM);
 	}

	memset(edit, 0, sizeof(*edit));

	err = _editor_init_ui(edit);

	if(err < 0) {
		editor_free(&edit);
		return(err);
	}

	*editor = edit;
	return(0);
}

int editor_free(struct editor **editor)
{
	if(!editor || !*editor) {
		return(-ENOMEM);
	}

	if((*editor)->window) {
		/*
		 * Because of the order in which functions are called during
		 * UI initialization, this is guaranteed to free all widgets.
		 */
		widget_free((struct widget*)(*editor)->window);
	}

	if((*editor)->prebuffer) {
		buffer_close(&((*editor)->prebuffer));
	}

	if((*editor)->postbuffer) {
		buffer_close(&((*editor)->postbuffer));
	}

	return(0);
}
