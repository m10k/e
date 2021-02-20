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
	}

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
