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
int telex_to_string(struct telex *telex, char *str, const size_t str_size);
int telex_append(struct telex *head, struct telex *tail);
int telex_clone(struct telex *src, struct telex **dst);

const char* telex_lookup(struct telex *telex, const char *start,
			 const size_t size, const char *pos);

#ifdef TELEX_DEBUG
int telex_debug(struct telex *expr, int depth)
#endif /* TELEX_DEBUG */

#endif /* E_TELEX_H */
