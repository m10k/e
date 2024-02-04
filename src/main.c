#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include "editor.h"

#define SHORTOPTS "f:hr"

static struct option _cmd_opts[] = {
	{ "debug", no_argument, 0, 'd' },
	{ "help", no_argument, 0, 'h' },
	{ "readonly", no_argument, 0, 'r' },
	{ 0, 0, 0, 0 }
};

static void _print_usage(const char *argv0)
{
	printf("Usage: %s [OPTIONS] filename\n"
	       "\n"
	       "Options:\n"
	       " -d  --debug         Print debug output to stderr\n"
	       " -h  --help          Display this help\n"
	       " -r  --readonly      Open file read-only\n",
	       argv0);
	return;
}

static int _disable_debug_output(void)
{
	int err;
	int devnull;

	if ((devnull = open("/dev/null", O_WRONLY)) < 0) {
		err = -errno;
		perror("open");
		return err;
	}

	if (close(2) < 0) {
		err = -errno;
		perror("close");
	} else if (dup2(devnull, 2) < 0) {
		err = -errno;
	} else {
		err = 0;
	}

	close(devnull);
	return 0;
}

int main(int argc, char *argv[])
{
	struct editor *editor;
	const char *filename;
	int readonly;
	int debug;
	int err;

	filename = NULL;
	readonly = 0;

	do {
		err = getopt_long(argc, argv, SHORTOPTS, _cmd_opts, NULL);

		switch(err) {
		case 'h':
			_print_usage(argv[0]);
			return(1);

		case 'r':
			readonly = 1;
			break;

		case 'd':
			debug = 1;
			break;

		case '?':
			fprintf(stderr, "Unrecognized parameter `%s'\n", optarg);
			return(1);

		default:
			err = -1;
			break;
		}
	} while(err >= 0);

	if (optind >= argc) {
		_print_usage(argv[0]);
		return 1;
	}

	if (!debug) {
		_disable_debug_output();
	}

	filename = argv[optind];
	err = editor_new(&editor);

	if(err < 0) {
		fprintf(stderr, "editor_new: %s\n", strerror(-err));
		return(1);
	}

	err = editor_open(editor, filename, readonly);

	if(!err) {
		editor_run(editor);
	} else {
		fprintf(stderr, "editor_open: %s\n", strerror(-err));
	}

	editor_free(&editor);

	return(err);
}
