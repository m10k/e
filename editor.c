#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <telex/telex.h>
#include "editor.h"
#include "buffer.h"
#include "string.h"
#include "ui.h"

struct editor {
	struct window *window;
	struct vbox *vbox;
	struct textview *edit;
	struct cmdbox *cmdbox;

	struct buffer *buffer;

	struct telex *sel_start;
	struct telex *sel_end;

	int readonly;
};

static int _cmdbox_set_text_from_telex(struct cmdbox *box, struct telex *telex)
{
	int max_chars;
	char *buffer;
	int err;

	if(!box || !telex) {
		return(-EINVAL);
	}

	max_chars = ((struct widget*)box)->width;
	buffer = calloc(max_chars + 1, sizeof(*buffer));

	if(!buffer) {
		return(-ENOMEM);
	}

	memset(buffer, 0, max_chars + 1);

	err = telex_to_string(telex, buffer, max_chars + 1);

	if(err >= 0) {
		cmdbox_set_text(box, buffer);
	}

	free(buffer);

	return(0);
}

static int _selection_start_change(struct widget *widget,
				void *user_data,
				void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	struct string *buffer;
	struct telex *telex;
	const char *expr_ptr;
	struct telex_error *errors;
	int err;

	if(!widget || !user_data || !data) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;
	buffer = (struct string*)data;

	/* turn off highlight, if any */
	cmdbox_highlight(box, UI_COLOR_DELETION, 0, 0);

	/* empty cmdbox? -> set text from current selection */
	if(cmdbox_get_length(box) == 0) {
		_cmdbox_set_text_from_telex(box, editor->sel_start);
		return(0);
	}

	expr_ptr = string_get_data(buffer);
	err = telex_parse(&telex, expr_ptr, &errors);

	if(err) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
	} else {
		const char *start;
		size_t size;

		start = buffer_get_data(editor->buffer);
		size = buffer_get_size(editor->buffer);

		if (!telex_lookup(telex, start, size, start)) {
			telex_free(&telex);
			cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		} else {
			telex_free(&editor->sel_start);
			editor->sel_start = telex;

			textview_set_selection_start(editor->edit, telex);
			cmdbox_clear(box);
		}
	}

	return(0);
}

static int _selection_end_change(struct widget *widget,
			      void *user_data,
			      void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	struct string *buffer;
	struct telex *telex;
	const char *expr_ptr;
	struct telex_error *errors;
	int err;

	if(!widget || !user_data || !data) {
		return(-EINVAL);
	}

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;
	buffer = (struct string*)data;

	/* turn off highlight, if any */
	cmdbox_highlight(box, UI_COLOR_DELETION, 0, 0);

	/* empty cmdbox? -> set text from current selection */
	if(cmdbox_get_length(box) == 0) {
		_cmdbox_set_text_from_telex(box, editor->sel_end);
		return(0);
	}

	expr_ptr = string_get_data(buffer);
	err = telex_parse(&telex, expr_ptr, &errors);

	if(err) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
	} else {
		const char *start;
		size_t size;

		start = buffer_get_data(editor->buffer);
		size = buffer_get_size(editor->buffer);

		if (!telex_lookup(telex, start, size, start)) {
			telex_free(&telex);
			cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		} else {
			telex_free(&editor->sel_end);
			editor->sel_end = telex;

			textview_set_selection_end(editor->edit, telex);
			cmdbox_clear(box);
		}
	}

	return 0;
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
	} else if((err = textview_new(&(editor->edit))) < 0) {
		return(err);
	} else if((err = container_add((struct container*)editor->vbox,
				       (struct widget*)editor->edit)) < 0) {
		return(err);
	} else if((err = cmdbox_new(&(editor->cmdbox))) < 0) {
		return(err);
	} else if((err = container_add((struct container*)editor->vbox,
				       (struct widget*)editor->cmdbox)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "selection_start_changed",
					    _selection_start_change,
					    editor)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "selection_end_changed",
					    _selection_end_change,
					    editor)) < 0) {
		return(err);
	}

	widget_resize((struct widget*)editor->window);
	widget_redraw((struct widget*)editor->window);

	return(0);
}

int editor_open(struct editor *editor, const char *path, const int readonly)
{
	int err;

	if(!editor || !path) {
		return(-EINVAL);
	}

	if(editor->buffer) {
		return(-EALREADY);
	}

	err = buffer_open(&(editor->buffer), path, readonly);

	if(err < 0) {
		return(err);
	}

	editor->readonly = readonly;

	err = textview_set_buffer(editor->edit, editor->buffer);

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
		} else if(event == KEY_RESIZE) {
			window_adjust_size(editor->window);
		} else {
			widget_input((struct widget*)editor->window, event);
		}
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

	edit = calloc(1, sizeof(*edit));

	if(!edit) {
		return(-ENOMEM);
 	}

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

	if((*editor)->buffer) {
		buffer_close(&((*editor)->buffer));
	}

	return(0);
}
