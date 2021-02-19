#ifndef E_TELEX_H
#define E_TELEX_H

#include "string.h"

typedef enum {
	TELEX_INVALID = 0,
        TELEX_NONE,
	TELEX_LINE,
	TELEX_REGEX,
	TELEX_COLUMN
} telex_type_t;

struct telex {
	struct telex *next;
	telex_type_t type;
	int direction;

	union {
		long number;
	        struct string *regex;
	} data;
};

int telex_parse(const char *str, struct telex **telex, const char **err_ptr);
int telex_free(struct telex **telex);

#ifdef TELEX_DEBUG
int telex_debug(struct telex *expr, int depth)
#endif /* TELEX_DEBUG */

#endif /* E_TELEX_H */
