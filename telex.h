#ifndef E_TELEX_H
#define E_TELEX_H

typedef enum {
	TELEX_INVALID = 0,
        TELEX_NONE,
	TELEX_LINE,
	TELEX_REGEX,
	TELEX_OFFSET
} telex_type_t;

struct telex {
	struct telex *next;
	telex_type_t type;
	int direction;

	union {
		long line;
		long offset;
		char *regex;
		long error;
	} data;
};

#endif /* E_TELEX_H */
