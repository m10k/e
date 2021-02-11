#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct term {
	int fd;
};

int term_alloc(struct term **dst)
{
	struct term *term;

	term = malloc(sizeof(*term));

	if(!term) {
		return(-ENOMEM);
	}

	memset(term, 0, sizeof(*term));

	term->fd = STDIN_FILENO;
	*dst = term;

	return(0);
}

int term_init(struct term *term)
{
	int err;

	if(!initscr()) {
		err = -EFAULT;
	} else if(raw() != OK) {
		err = -EIO;
	} else if(noecho() != OK) {
		err = -EIO;
	} else {
		err = 0;
	}

	if(err < 0 && err != -EFAULT) {
		endwin();
	}

	return(err);
}

int term_fini(struct term *term)
{
	if(endwin() != OK) {
		return(-EIO);
	}

	return(0);
}

int term_free(struct term **term)
{
	if(!term || !*term) {
		return(-EINVAL);
	}

	memset(*term, 0, sizeof(**term));
	free(*term);

	return(0);
}

int term_new(struct term **dst)
{
	struct term *term;

	if(!dst) {
		return(-EINVAL);
	}

	if(term_alloc(&term) < 0) {
		return(-ENOMEM);
	}

	if(term_init(term) < 0) {
		term_free(&term);
		return(-EIO);
	}

	*dst = term;
	return(0);
}

int term_destroy(struct term **term)
{
	if(!term || !*term) {
		return(-EINVAL);
	}

	if(term_fini(*term) < 0) {
		return(-EIO);
	}

	return(term_free(term));
}
