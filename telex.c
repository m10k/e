#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "telex.h"

static int _telex_new(struct telex **dst)
{
	struct telex *seg;

	if(!dst) {
		return(-EINVAL);
	}

	seg = malloc(sizeof(*seg));

	if(!seg) {
		return(-ENOMEM);
	}

	memset(seg, 0, sizeof(*seg));
	*dst = seg;

	return(0);
}

static int _telex_free(struct telex **telex)
{
	if(!telex || !*telex) {
		return(-EINVAL);
	}

	free(*telex);
	*telex = NULL;

	return(0);
}

int telex_parse(const char *str, struct telex **dst)
{
	struct telex *exp;
	const char *pos;
	int escape;
	int stop;

	if(_telex_new(&exp) < 0) {
		return(-ENOMEM);
	}

	exp->direction = 1;
	exp->type = TELEX_NONE;
	stop = 0;
	escape = 0;

	for(pos = str; *pos && !stop; pos++) {
		switch(exp->type) {
		case TELEX_NONE:
			switch(*pos) {
			case '-':
				exp->direction = -1;
			case '+':
				break;

			case '<':
				exp->direction = -1;
			case '>':
				exp->type = TELEX_OFFSET;
				break;

			case '"':
				exp->type = TELEX_REGEX;
				break;

			default:
				if(isdigit(*pos)) {
					exp->type = TELEX_LINE;
				} else {
					exp->type = TELEX_INVALID;
					exp->data.error = pos - str;
					stop = 1;
				}
				break;
			}
			break;

		case TELEX_INVALID:
			stop = 1;
			break;

		case TELEX_LINE:
			if(isdigit(*pos)) {
				exp->data.line = exp->data.line * 10 +
					*pos - '0';
			} else {
				stop = 1;
			}

			break;

		case TELEX_REGEX:
			if(!escape) {
				if(*pos == '"') {
					stop = 1;
					break;
				}

				if(*pos == '\\') {
					escape = 1;
					break;
				}
			}

			/* add to string */
			escape = 0;
			break;

		case TELEX_OFFSET:
			if(isdigit(*pos)) {
				exp->data.offset = exp->data.offset * 10 +
					*pos - '0';
			} else {
				stop = 1;
			}

			break;
		}
	}

	return(0);
}
