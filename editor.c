#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <telex/telex.h>
#include "editor.h"
#include "buffer.h"
#include "string.h"
#include "ui.h"

/* FIXME: Variables should be stored in a hashmap once we have one */
struct variable {
	struct variable *next;
	char *name;
	char *value;
};

struct editor {
	struct window *window;
	struct vbox *vbox;
	struct textview *edit;
	struct cmdbox *cmdbox;

	struct buffer *buffer;

	struct telex *sel_start;
	struct telex *sel_end;

	struct variable *variables;

	int readonly;
};

struct variable* _editor_find_variable(struct editor *editor, const char *name);

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
	if (cmdbox_get_length(box) == 0) {
		_cmdbox_set_text_from_telex(box, editor->sel_start);
		textview_set_selection_start(editor->edit, NULL);
		return 0;
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
	if (cmdbox_get_length(box) == 0) {
		_cmdbox_set_text_from_telex(box, editor->sel_end);
		textview_set_selection_end(editor->edit, NULL);
		return 0;
	}

	expr_ptr = string_get_data(buffer);
	err = telex_parse(&telex, expr_ptr, &errors);

	if (err) {
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

static int _oinsert_requested(struct widget *widget,
				void *user_data,
				void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	const char *insertion;
	int err;

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;

	if (!editor->sel_start) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	if (cmdbox_get_length(box) == 0) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	insertion = cmdbox_get_text(box);
	if ((err = buffer_overwrite(editor->buffer, insertion, editor->sel_start, editor->sel_end)) < 0) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	cmdbox_clear(box);
	widget_redraw((struct widget*)editor->window);

	return 0;
}

static int _insert_requested(struct widget *widget,
			     void *user_data,
			     void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	const char *insertion;
	int err;

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;

	if (!editor->sel_start) {
		fprintf(stderr, "Don't know where to insert text\n");
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	if (cmdbox_get_length(box) == 0) {
		fprintf(stderr, "Nothing to insert\n");
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	insertion = cmdbox_get_text(box);
	if ((err = buffer_insert(editor->buffer, insertion, editor->sel_start)) < 0) {
		fprintf(stderr, "Could not insert text: %s [%d]\n", strerror(-err), -err);
		return 0;
	}

	/* TODO: Advance selection so the user can insert more text */

	cmdbox_clear(box);
	widget_redraw((struct widget*)editor->window);

	return 0;
}

static int _write_requested(struct widget *widget,
			    void *user_data,
			    void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	const char *var_name;
	const char *var_value;
	int err;

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;

	if (!editor->sel_start) {
		fprintf(stderr, "Don't know where to insert variable\n");
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	if (cmdbox_get_length(box) == 0) {
		fprintf(stderr, "Don't know which variable to insert\n");
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	var_name = cmdbox_get_text(box);
	if (editor_get_var(editor, var_name, &var_value) < 0) {
		fprintf(stderr, "No variable \"%s\"\n", var_name);
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, strlen(var_name));
		return 0;
	}

	fprintf(stderr, "Inserting variable \"%s\" into buffer\n", var_name);
	if ((err = buffer_insert(editor->buffer, var_value, editor->sel_start)) < 0) {
		fprintf(stderr, "Could not insert variable \"%s\": %s [%d]\n",
			var_name, strerror(-err), -err);
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return 0;
	}

	cmdbox_clear(box);
	widget_redraw((struct widget*)editor->window);

	return 0;
}

static int _read_requested(struct widget *widget,
			   void *user_data,
			   void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	const char *substring;
	size_t substring_len;
	const char *var;
	int err;

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;

        cmdbox_highlight(box, UI_COLOR_DELETION, 0, 0);

        if(cmdbox_get_length(box) == 0) {
		/* user didn't enter a variable name */
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
                return 0;
        }

	var = cmdbox_get_text(box);

	/*
	 * Read the data that the selection points to and save it in a
	 * variable with the name in `dst'.
	 */

	if ((err = buffer_get_substring(editor->buffer, editor->sel_start, editor->sel_end,
					&substring, &substring_len)) < 0) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return err;
	}

	fprintf(stderr, "Setting variable \"%s\" to \"%s\"\n", var, substring);

	if ((err = editor_set_var(editor, var, substring)) < 0) {
		free((void*)substring);
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return err;
	}

	cmdbox_clear(box);
	return 0;
}

static int _save_requested(struct widget *widget,
			   void *user_data,
			   void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	int err;

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;

	if ((err = buffer_save(editor->buffer)) < 0) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return err;
	}

	return 0;
}

static int _erase_requested(struct widget *widget,
			    void *user_data,
			    void *data)
{
	struct cmdbox *box;
	struct editor *editor;
	int err;

	box = (struct cmdbox*)widget;
	editor = (struct editor*)user_data;

	if ((err = buffer_erase(editor->buffer, editor->sel_start, editor->sel_end)) < 0) {
		cmdbox_highlight(box, UI_COLOR_DELETION, 0, -1);
		return err;
	}

	widget_redraw((struct widget*)editor->window);
	return 0;
}

struct variable* _editor_find_variable(struct editor *editor, const char *name)
{
	struct variable *var;

	for (var = editor->variables; var; var = var->next) {
		if (strcmp(var->name, name) == 0) {
			return var;
		}
	}

	return NULL;
}

int _variable_free(struct variable **var)
{
	struct variable *next;

	if (!var || !*var) {
		return -EINVAL;
	}

	if ((*var)->name) {
		free((*var)->name);
	}
	if ((*var)->value) {
		free((*var)->value);
	}

	next = (*var)->next;
	free(*var);
	*var = next;

	return 0;
}

struct variable* _variable_new(const char *name, const char *value)
{
	struct variable *var;

	if ((var = calloc(1, sizeof(*var)))) {
		if (!(var->name = strdup(name)) ||
		    !(var->value = strdup(value))) {
			_variable_free(&var);
		}
	}

	return var;
}

int _variable_set(struct variable *var, const char *value)
{
	char *value_dup;

	if (!(value_dup = strdup(value))) {
		return -ENOMEM;
	}

	free(var->value);
	var->value = value_dup;

	return 0;
}

int editor_set_var(struct editor *editor, const char *name, const char *value)
{
	struct variable *var;

	if ((var = _editor_find_variable(editor, name))) {
		return _variable_set(var, value);
	}

	if (!(var = _variable_new(name, value))) {
		return -ENOMEM;
	}

	var->next = editor->variables;
	editor->variables = var;

	return 0;
}

int editor_get_var(struct editor *editor, const char *name, const char **value)
{
	struct variable *var;

	if (!(var = _editor_find_variable(editor, name))) {
		return -ENOENT;
	}

	*value = var->value;
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
	} else if ((err = widget_add_handler((struct widget*)editor->cmdbox,
					     "read_requested",
					     _read_requested,
					     editor)) < 0) {
		return err;
	} else if ((err = widget_add_handler((struct widget*)editor->cmdbox,
					     "write_requested",
					     _write_requested,
					     editor)) < 0) {
		return err;
	} else if ((err = widget_add_handler((struct widget*)editor->cmdbox,
					     "insert_requested",
					     _insert_requested,
					     editor)) < 0) {
		return err;
	} else if ((err = widget_add_handler((struct widget*)editor->cmdbox,
					     "oinsert_requested",
					     _oinsert_requested,
					     editor)) < 0) {
		return err;
	} else if ((err = widget_add_handler((struct widget*)editor->cmdbox,
					     "save_requested",
					     _save_requested,
					     editor)) < 0) {
		return err;
	} else if ((err = widget_add_handler((struct widget*)editor->cmdbox,
					     "erase_requested",
					     _erase_requested,
					     editor)) < 0) {
		return err;
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
