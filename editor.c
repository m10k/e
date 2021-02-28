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

	struct telex *source_start;
	struct telex *source_end;
	struct telex *destination_start;
	struct telex *destination_end;

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
	buffer = malloc(max_chars + 1);

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

static int _try_append_telex(struct telex *head, struct telex *tail,
			     struct buffer *buffer, struct telex **dest)
{
	const char *start;
	size_t size;

	if(!head || !tail || !buffer) {
		return(-EINVAL);
	}

	start = buffer_get_data(buffer);
	size = buffer_get_size(buffer);

	if(!telex_lookup_multi(start, size, NULL, 2, head, tail)) {
	        return(-EBADMSG);
	}

	if(dest) {
		/*
		 * If dest was provided, we're supposed to
		 * clone before appending
		 */
		if(telex_clone(head, &head) < 0) {
			return(-ENOMEM);
		}

		*dest = head;
	}

	return(telex_append(head, tail));
}

static int _source_start_change(struct widget *widget,
				void *user_data,
				void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	struct string *buffer;
	struct telex *telex;
	const char *expr_ptr;
	const char *err_ptr;
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
		_cmdbox_set_text_from_telex(box, editor->source_start);
		return(0);
	}

	expr_ptr = string_get_data(buffer);
	err = telex_parse(expr_ptr, &telex, &err_ptr);

	if(err) {
		cmdbox_highlight(box, UI_COLOR_DELETION,
				 err_ptr ? (int)(err_ptr - expr_ptr) : 0,
				 err_ptr ? (int)strlen(err_ptr) : -1);
	} else {
		const char *start;
		size_t size;

		start = buffer_get_data(editor->prebuffer);
		size = buffer_get_size(editor->prebuffer);

		if(telex->direction && editor->source_start) {
			err = _try_append_telex(editor->source_start, telex,
						editor->prebuffer, NULL);

			if(err < 0) {
				cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
				telex_free(&telex);
			} else {
				cmdbox_clear(box);
				widget_redraw((struct widget*)editor->preedit);
			}

			return(err);
		}

		if(!telex_lookup(telex, start, size, start)) {
			cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
			return(0);
		}

		telex_free(&(editor->source_start));
		editor->source_start = telex;

		textview_set_selection_start(editor->preedit, telex);

		cmdbox_clear(box);
	}

	return(0);
}

static int _source_end_change(struct widget *widget,
			      void *user_data,
			      void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	struct string *buffer;
	struct telex *telex;
	const char *expr_ptr;
	const char *err_ptr;
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
		_cmdbox_set_text_from_telex(box, editor->source_end);
		return(0);
	}

	expr_ptr = string_get_data(buffer);
	err = telex_parse(expr_ptr, &telex, &err_ptr);

	if(err) {
		cmdbox_highlight(box, UI_COLOR_DELETION,
				 err_ptr ? (int)(err_ptr - expr_ptr) : 0,
				 err_ptr ? (int)strlen(err_ptr) : -1);
	} else {
		const char *start;
		size_t size;

		start = buffer_get_data(editor->prebuffer);
		size = buffer_get_size(editor->prebuffer);

		if(telex->direction && editor->source_end) {
			/*
			 * Try to make this expression relative to the
			 * previous selection end.
			 */
			err = _try_append_telex(editor->source_end, telex,
						editor->prebuffer, NULL);

			if(err < 0) {
				cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
				telex_free(&telex);
			} else {
				cmdbox_clear(box);
				widget_redraw((struct widget*)editor->preedit);
			}

			return(err);
		} else if(telex->direction && editor->source_start) {
			/*
			 * If end of the selection isn't set, try to make this
			 * expression relative to the selection start
			 */

			err = _try_append_telex(editor->source_start, telex,
						editor->prebuffer, &(editor->source_end));

			if(err < 0) {
				cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
				telex_free(&telex);
			} else {
				cmdbox_clear(box);
				textview_set_selection_end(editor->preedit, editor->source_end);
				widget_redraw((struct widget*)editor->preedit);
			}

			return(err);
		}

		if(!telex_lookup(telex, start, size, start)) {
			cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
			return(0);
		}

		telex_free(&(editor->source_end));
		editor->source_end = telex;

		textview_set_selection_end(editor->preedit, telex);

		cmdbox_clear(box);
	}

	return(0);
}

static int _destination_start_change(struct widget *widget,
				     void *user_data,
				     void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	struct string *buffer;
	struct telex *telex;
	const char *expr_ptr;
	const char *err_ptr;
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
		_cmdbox_set_text_from_telex(box, editor->destination_start);
		return(0);
	}

	expr_ptr = string_get_data(buffer);
	err = telex_parse(expr_ptr, &telex, &err_ptr);

	if(err) {
		cmdbox_highlight(box, UI_COLOR_DELETION,
				 err_ptr ? (int)(err_ptr - expr_ptr) : 0,
				 err_ptr ? (int)strlen(err_ptr) : -1);
	} else {
		const char *start;
		size_t size;

		start = buffer_get_data(editor->postbuffer);
		size = buffer_get_size(editor->postbuffer);

		if(telex->direction && editor->destination_start) {
			err = _try_append_telex(editor->destination_start, telex,
						editor->postbuffer, NULL);

			if(err < 0) {
				cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
				telex_free(&telex);
			} else {
				cmdbox_clear(box);
				widget_redraw((struct widget*)editor->postedit);
			}

			return(err);
		}

		if(!telex_lookup(telex, start, size, start)) {
			cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
			return(0);
		}

		telex_free(&(editor->destination_start));
		editor->destination_start = telex;

		widget_set_visible((struct widget*)editor->postedit, TRUE);
		textview_set_selection_start(editor->postedit, telex);

		cmdbox_clear(box);
	}

	return(0);
}

static int _destination_end_change(struct widget *widget,
				   void *user_data,
				   void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	struct string *buffer;
	struct telex *telex;
	const char *expr_ptr;
	const char *err_ptr;
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
		_cmdbox_set_text_from_telex(box, editor->destination_end);
		return(0);
	}

	expr_ptr = string_get_data(buffer);
	err = telex_parse(expr_ptr, &telex, &err_ptr);

	if(err) {
		cmdbox_highlight(box, UI_COLOR_DELETION,
				 err_ptr ? (int)(err_ptr - expr_ptr) : 0,
				 err_ptr ? (int)strlen(err_ptr) : -1);
	} else {
		const char *start;
		size_t size;

		start = buffer_get_data(editor->postbuffer);
		size = buffer_get_size(editor->postbuffer);

		if(telex->direction && editor->destination_end) {
			/*
			 * Try to make this expression relative to the
			 * previous selection end.
			 */
			err = _try_append_telex(editor->destination_end, telex,
						editor->postbuffer, NULL);

			if(err < 0) {
				cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
				telex_free(&telex);
			} else {
				cmdbox_clear(box);
				widget_redraw((struct widget*)editor->postedit);
			}

			return(err);
		} else if(telex->direction && editor->destination_start) {
			/*
			 * If end of the selection isn't set, try to make this
			 * expression relative to the selection start
			 */

			err = _try_append_telex(editor->destination_start, telex,
						editor->postbuffer, &(editor->destination_end));

			if(err < 0) {
				cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
				telex_free(&telex);
			} else {
				cmdbox_clear(box);
				textview_set_selection_end(editor->postedit, editor->destination_end);
				widget_redraw((struct widget*)editor->postedit);
			}

			return(err);
		}

		if(!telex_lookup(telex, start, size, start)) {
			cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
			return(0);
		}

		telex_free(&(editor->destination_end));
		editor->destination_end = telex;

		widget_set_visible((struct widget*)editor->postedit, TRUE);
		textview_set_selection_end(editor->postedit, telex);

		cmdbox_clear(box);
	}

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
					    _source_start_change,
					    editor)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "source_end_changed",
					    _source_end_change,
					    editor)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "destination_start_changed",
					    _destination_start_change,
					    editor)) < 0) {
		return(err);
	} else if((err = widget_add_handler((struct widget*)editor->cmdbox,
					    "destination_end_changed",
					    _destination_end_change,
					    editor)) < 0) {
		return(err);
	}

	widget_set_visible((struct widget*)editor->postedit, FALSE);

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

	if(editor->prebuffer) {
		return(-EALREADY);
	}

	err = buffer_open(&(editor->prebuffer), path, readonly);

	if(err < 0) {
		return(err);
	}

	editor->readonly = readonly;

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
