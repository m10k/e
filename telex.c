#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include "telex.h"
#include "string.h"
#include "file.h"

/*
 * Text Location Expressions
 *
 * A telex is an expression determining a location in a text file.
 * The simplest kind of telex are absolute line expressions:
 *
 *   1           Start of the first line (telex are not 0-based!)
 *   123         Start of line 123
 *   0           Start of the last line
 *
 * Telex can also be relative:
 *
 *   +1          Next line
 *   -1          Previous line
 *
 * Telex can be stacked, like this:
 *
 *   1+10        Start of the 11th line (start of the first, then move
 *               10 lines down)
 *   0-1         Start of the penultimate line
 *
 * However, lines are not wrapped, i.e. you cannot move to the last line
 * by making a relative movement such as `1-1'.
 *
 * The # operator can be used to move within lines:
 *
 *   #10         current line, column 10
 *   +#10        current line, move 10 columns right
 *   -#10        current line, move 10 columns left
 *   123#10      Line 123, column 10
 *   12#10-#5    Line 12, column 5
 *   10#11+#12   Line 10, column 23
 *   10#0        Line 10, last column
 *
 * What we have seen so far are absolute and relative expressions for
 * line and column movement. To generalize the previous, numbers that are
 * not prefixed with + or - indicate an absolute position. Prefixed numbers
 * indicate a movement relative to the current position.
 *
 * The above might be useful for scripting, but humans editing a text file
 * usually don't think in line and column numbers. For humans, tokens are
 * more natural.
 *
 *   +"struct"       The next occurence of the token "struct"
 *   +"struct"+"^};" The first occurence of the token "};" at the start of a
 *                   line after the next occurence of the token "struct"
 *
 */

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

int telex_free(struct telex **telex)
{
	if(!telex || !*telex) {
		return(-EINVAL);
	}

	if((*telex)->next) {
		telex_free(&((*telex)->next));
	}

	free(*telex);
	*telex = NULL;

	return(0);
}

typedef enum {
	STATE_INIT = 0,
	STATE_NUMERIC,
	STATE_REGEX,
	STATE_REGEX_ESC,
	STATE_DONE,
	NUM_STATES
} parser_state_t;

typedef parser_state_t (_parser_func)(const char, struct telex*);

static parser_state_t _telex_parse_state_init(const char cur,
					      struct telex *expr)
{
        parser_state_t next_state;

	switch(cur) {
	case '+':
	case '-':
		/* relative movement */
		expr->direction = cur == '+' ? 1 : -1;
		next_state = STATE_INIT;
		break;

	case '#':
		/* column expression */
		expr->type = TELEX_COLUMN;
		expr->data.number = 0;
		next_state = STATE_NUMERIC;
		break;

	case '"':
		/* regex expression */
		expr->type = TELEX_REGEX;
		next_state = STATE_REGEX;
		break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		/* line movement */
		expr->type = TELEX_LINE;
		expr->data.number = cur - '0';
		next_state = STATE_NUMERIC;
		break;

	default:
		expr->type = TELEX_INVALID;
		next_state = STATE_DONE;
	}

	return(next_state);
}

static parser_state_t _telex_parse_state_numeric(const char cur,
						 struct telex *expr)
{
	parser_state_t next_state;

	if(isdigit(cur)) {
		expr->data.number = expr->data.number * 10 + cur - '0';
		next_state = STATE_NUMERIC;
	} else {
		next_state = STATE_DONE;
	}

	return(next_state);
}

static int _expr_append_char(struct telex *expr, const char chr)
{
	if(!expr->data.regex &&
	   string_new(&(expr->data.regex)) < 0) {
		return(-ENOMEM);
	}

	if(string_insert_char(expr->data.regex, -1, chr) < 0) {
		return(-ENOMEM);
	}

	return(0);
}

static parser_state_t _telex_parse_state_regex(const char cur,
					       struct telex *expr)
{
	parser_state_t next_state;

	switch(cur) {
	case '\\':
		next_state = STATE_REGEX_ESC;
		break;

	case 0:
		expr->type = TELEX_INVALID;
	case '"':
		next_state = STATE_DONE;
		break;

	default:
		next_state = STATE_REGEX;

		if(_expr_append_char(expr, cur) < 0) {
			next_state = STATE_DONE;
		}

		break;
	}

	return(next_state);
}

static parser_state_t _telex_parse_state_regex_esc(const char cur,
						   struct telex *expr)
{
	parser_state_t next_state;

	switch(cur) {
	case 0:
		expr->type = TELEX_INVALID;
		next_state = STATE_DONE;
		break;

	default:
		next_state = STATE_REGEX;

		if(_expr_append_char(expr, cur) < 0) {
			next_state = STATE_DONE;
		}

		break;
	}

	return(next_state);
}

int telex_parse_expr(const char *str, struct telex **telex, const char **next)
{
	static _parser_func *_parser_funcs[NUM_STATES] = {
		_telex_parse_state_init,
		_telex_parse_state_numeric,
		_telex_parse_state_regex,
		_telex_parse_state_regex_esc,
		NULL
	};

	parser_state_t state;
	struct telex *expr;
	const char *cur;

	if(!str || !telex || !next) {
		return(-EINVAL);
	}

	if(_telex_new(&expr) < 0) {
		return(-ENOMEM);
	}

	for(cur = str, state = STATE_INIT; state < STATE_DONE; cur++) {
		state = _parser_funcs[state](*cur, expr);
	}

	*next = cur - 1;

	if(expr->type == TELEX_INVALID) {
		telex_free(&expr);
		return(-EBADMSG);
	}

	*telex = expr;

	return(0);
}

int telex_parse(const char *str, struct telex **telex, const char **err_ptr)
{
	const char *cur_pos;
	struct telex *first_expr;
	struct telex *prev_expr;
	int num_exprs;
	int err;

	first_expr = NULL;
	prev_expr = NULL;
	cur_pos = str;
	num_exprs = 0;

	if(!str || !telex) {
		return(-EINVAL);
	}

	err = -EBADMSG;
	if(err_ptr) {
		*err_ptr = str;
	}

	while(*cur_pos) {
		struct telex *cur_expr;
		const char *next_pos;

		err = telex_parse_expr(cur_pos, &cur_expr, &next_pos);

		if(err < 0) {
			if(err == -EBADMSG && err_ptr) {
				*err_ptr = next_pos;
			}

			break;
		}

		if(!first_expr) {
			first_expr = cur_expr;
		}

		if(prev_expr) {
			prev_expr->next = cur_expr;
		}
		prev_expr = cur_expr;

		cur_pos = next_pos;
		num_exprs++;
	}

	if(first_expr) {
		if(err < 0) {
			telex_free(&first_expr);
		} else {
			*telex = first_expr;
		}
	}

	return(err);
}

const char *telex_type_str(telex_type_t type)
{
	static const char *_telex_type_strs[] = {
		"invalid",
		"none",
		"line",
		"regex",
		"column",
		NULL
	};

	if(type < TELEX_INVALID || type > TELEX_COLUMN) {
		type = TELEX_INVALID;
	}

	return(_telex_type_strs[type]);
}

#ifdef TELEX_DEBUG
int telex_debug(struct telex *expr, int depth)
{
	char *spacing;

	spacing = malloc(depth * 2 + 1);

	if(!spacing) {
		return(-ENOMEM);
	}

	memset(spacing, ' ', depth * 2);
	spacing[depth * 2] = 0;

	printf("%sTELEX type=%s\n"
	       "%sdir=%d\n",
	       spacing, telex_type_str(expr->type),
	       spacing, expr->direction);

	switch(expr->type) {
	case TELEX_LINE:
	case TELEX_COLUMN:
		printf("%snumber=%lu\n", spacing, expr->data.number);
		break;

	case TELEX_REGEX:
		printf("%sregex=%s\n", spacing, string_get_data(expr->data.regex));
		break;

	case TELEX_INVALID:
	case TELEX_NONE:
		printf("%s[INVALID]\n", spacing);
		break;
	}

	free(spacing);

	if(expr->next) {
		telex_debug(expr->next, depth + 1);
	}

	free(expr);
	return(0);
}
#endif /* TELEX_DEBUG */

static const char *rstrchr(const char *base, const char *start, const char chr)
{
	while(start >= base) {
		if(*start == chr) {
			return(start);
		}

		start--;
	}

	return(NULL);
}

static const char* _telex_lookup_line(struct telex *telex, const char *start,
				      const size_t size, const char *pos)
{
	const char *cur;
	long remaining;

	remaining = telex->data.number;
	cur = pos;

	if(telex->direction == 0 &&
	   telex->data.number == 0) {
		cur = start + size;
		telex->direction = -1;
		telex->data.number = 1;
	}

	if(telex->direction == 0) {
		cur = start;

		while(--remaining > 0) {
			cur = strchr(cur, '\n');

			if(!cur) {
				return(NULL);
			}

			cur++;
		}
	} else if(telex->direction > 0) {
		while(remaining-- > 0) {
			cur = strchr(cur, '\n');

			if(!cur) {
				return(NULL);
			}

			cur++;
		}
	} else { /* direction < 0 */
		while(remaining-- >= 0) {
			cur = rstrchr(start, cur, '\n');

			if(!cur) {
				return(NULL);
			}

			cur--;
		}

		cur += 2;
	}

	return(cur);
}

static const char* _telex_lookup_regex(struct telex *telex, const char *start,
				       const size_t size, const char *pos)
{
	return(NULL);
}

static const char* _telex_lookup_column(struct telex *telex, const char *start,
					const size_t size, const char *pos)
{
	const char *cur;
	const char *lim;
	long remaining;
	int step;

	step = telex->direction;
	remaining = telex->data.number;
	lim = (telex->direction > 0) ? start + size : start - 1;
	cur = pos;

	return(NULL);
}

const char* telex_lookup(struct telex *telex, const char *start,
			 const size_t size, const char *pos)
{
	const char *cur_pos;

	for(cur_pos = pos; telex; telex = telex->next) {
		const char *new_pos;

		switch(telex->type) {
		case TELEX_LINE:
			new_pos = _telex_lookup_line(telex, start, size, cur_pos);
			break;

		case TELEX_REGEX:
			new_pos = _telex_lookup_regex(telex, start, size, cur_pos);
			break;

		case TELEX_COLUMN:
			new_pos = _telex_lookup_column(telex, start, size, cur_pos);
			break;

		default:
			return(NULL);
		}

		cur_pos = new_pos;
	}

	return(cur_pos);
}
