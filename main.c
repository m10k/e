#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "editor.h"

static void _print_usage(const char *argv0)
{
	printf("Usage: %s filename\n", argv0);
	return;
}

int main(int argc, char *argv[])
{
	struct editor *editor;
	int err;

	if(argc < 2) {
		_print_usage(argv[0]);
		return(1);
	}

	err = editor_new(&editor);

	if(err < 0) {
		fprintf(stderr, "editor_new: %s\n", strerror(-err));
		return(1);
	}

	err = editor_open(editor, argv[1]);

	if(!err) {
		editor_run(editor);
	} else {
		fprintf(stderr, "editor_open: %s\n", strerror(-err));
	}

	editor_free(&editor);

	return(err);
}
