#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "string.h"
#include "multistring.h"

struct multistring {
	struct string **strings;
	int num_strings;
	int max_strings;
};

static int _defrag_string_array(struct multistring *mstr)
{
	int i;

	/* this assumes there will never be two or more adjacent NULLs in the middle of the array */

	for (i = 0; i < mstr->max_strings - 1; i++) {
		if (!mstr->strings[i]) {
			memmove(&(mstr->strings[i]), &(mstr->strings[i + 1]),
				(mstr->max_strings - i - 1) * sizeof(*mstr->strings));
		}
	}

	return 0;
}

static int _adjust_string_array_size(struct multistring *mstr, int size_change)
{
	struct string **new_strings;
	int new_max_strings;
	int err;

	new_max_strings = mstr->max_strings + size_change;

	if (new_max_strings < 0) {
		return -EOVERFLOW;
	}

	if (new_max_strings < mstr->num_strings) {
		return -EBUSY;
	}

	if (size_change < 0) {
		int line;

		if ((err = _defrag_string_array(mstr)) < 0) {
			return err;
		}

		for (line = mstr->max_strings + size_change; line < mstr->max_strings; line++) {
			if (mstr->strings[line]) {
				string_free(&mstr->strings[line]);
				mstr->num_strings--;
			}
		}
	}

	new_strings = realloc(mstr->strings, new_max_strings * sizeof(*mstr->strings));

	if (!new_strings) {
		return -ENOMEM;
	}

	if (size_change > 0) {
		memset(&(new_strings[mstr->max_strings]), 0, size_change * sizeof(*new_strings));
	}

	mstr->strings = new_strings;
	mstr->max_strings = new_max_strings;

	return 0;
}

static int _insert_lines_at(struct multistring *mstr, int line, int num_lines)
{
	int err;
	int empty_lines;
	int needed_lines;
	int copy_lines;
	int l;

	if (!mstr || num_lines < 0) {
		return -EINVAL;
	}

	if (line < 0) {
		line = mstr->num_strings;
	}

	empty_lines = mstr->max_strings - mstr->num_strings;
	needed_lines = num_lines - empty_lines;
	copy_lines = mstr->num_strings - line;

	if ((err = _adjust_string_array_size(mstr, needed_lines)) < 0) {
		return err;
	}

	memmove(&(mstr->strings[line + num_lines]), &(mstr->strings[line]),
		copy_lines * sizeof(*mstr->strings));

	for (l = line; l < line + num_lines; l++) {
		if (string_new(&(mstr->strings[l])) == 0) {
			mstr->num_strings++;
		}
	}

	return line;
}

int multistring_new(struct multistring **dst)
{
	struct multistring *mstr;
	int err;

	if (!(mstr = calloc(1, sizeof(*mstr)))) {
		return -ENOMEM;
	}

	if ((err = _insert_lines_at(mstr, 0, 1)) < 0) {
		free(mstr);
	} else {
		*dst = mstr;
		err = 0;
	}

	return err;
}

int multistring_free(struct multistring **mstr)
{
	if (!mstr || !*mstr) {
		return -EINVAL;
	}

	if ((*mstr)->strings) {
		int line;

		for (line = 0; line < (*mstr)->max_strings; line++) {
			string_free(&(*mstr)->strings[line]);
		}

		free((*mstr)->strings);
		(*mstr)->strings = NULL;
	}

	free(*mstr);
	*mstr = NULL;

	return 0;
}

int multistring_line_get_length(struct multistring *mstr, int line)
{
	if (!mstr) {
		return -EINVAL;
	}

	if (line < 0 || line > mstr->num_strings) {
		return -EOVERFLOW;
	}

	return string_get_length(mstr->strings[line]);
}

int multistring_get_length(struct multistring *mstr)
{
	int line;
	int length;

	for (length = 0, line = 0; line < mstr->num_strings; line++) {
		length += string_get_length(mstr->strings[line]);
	}

	return length;
}

static int _insert_newline(struct multistring *mstr, int line, int column)
{
	int err;

	fprintf(stderr, "_insert_newline(%p, %d, %d)\n", (void*)mstr, line, column);

	if (!mstr) {
		return -EINVAL;
	}

	if ((err = _insert_lines_at(mstr, line + 1, 1)) < 0) {
		return err;
	}

	if (!(mstr->strings[line + 1] = string_get_substring(mstr->strings[line], column, -1))) {
		return -errno;
	}

	if ((err = string_truncate(mstr->strings[line], column)) < 0) {
		/* FIXME: Need to undo changes */
		return err;
	}

	if ((err = string_insert_char(mstr->strings[line], -1, '\n')) < 0) {
		return err;
	}

	return 0;
}

int multistring_line_insert_char(struct multistring *mstr, int line, int col, char chr)
{
	fprintf(stderr, "%s(%p, %d, %d, '%c')\n", __func__, (void*)mstr, line, col, chr);

	if (!mstr) {
		return -EINVAL;
	}

	if (line < 0 || line >= mstr->num_strings) {
		return -EOVERFLOW;
	}

	if (chr == '\n') {
		return _insert_newline(mstr, line, col);
	}

	fprintf(stderr, "string_insert_char(mstr->strings[%d], %d, '%c')\n",
		line, col, chr);
	return string_insert_char(mstr->strings[line], col, chr);
}

int multistring_line_remove_char(struct multistring *mstr, int line, int col)
{
	const char *data;
	int is_newline;
	int err;

	if (!mstr) {
		return -EINVAL;
	}

	if (line < 0 || line >= mstr->num_strings) {
		return -ERANGE;
	}

	data = string_get_data(mstr->strings[line]);
	is_newline = data[col] == '\n';

	if ((err = string_remove_char(mstr->strings[line], col)) < 0) {
		return err;
	}

	if (is_newline) {
		/* append next line to current one */
		if ((err = string_insert_string(mstr->strings[line], -1,
						mstr->strings[line + 1])) < 0) {
			return err;
		}

		if ((err = multistring_line_remove(mstr, line + 1)) < 0) {
			return err;
		}
	}

	return 0;
}

int multistring_line_truncate(struct multistring *mstr, int line, int col)
{
	if (!mstr) {
		return -EINVAL;
	}

	if (line < 0 || line >= mstr->num_strings) {
		return -ERANGE;
	}

	return string_truncate(mstr->strings[line], col);
}

int multistring_line_remove(struct multistring *mstr, int line)
{
	int target;
	int err;

	if (!mstr) {
		return -EINVAL;
	}

	if (line < -1 || line >= mstr->num_strings) {
		return -ERANGE;
	}

	if (mstr->num_strings == 0) {
		return -EALREADY;
	}

	target = line == -1 ? mstr->num_strings - 1 : line;

	if ((err = string_free(&(mstr->strings[target]))) < 0) {
		return err;
	}
	mstr->num_strings--;

	return _adjust_string_array_size(mstr, -1);
}

const char* multistring_line_get_data(struct multistring *mstr, int line)
{
	if (!mstr || line < 0 || line >= mstr->num_strings) {
		return NULL;
	}

	return string_get_data(mstr->strings[line]);
}

char* multistring_get_data(struct multistring *mstr)
{
	char *data;
	int data_len;
	int offset;
	int line;

	data_len = 1 + multistring_get_length(mstr);

	if (!(data = calloc(data_len, 1))) {
		return NULL;
	}

	for (offset = 0, line = 0; line < mstr->num_strings; line++) {
		int written;
		const char *line_data;

		line_data = multistring_line_get_data(mstr, line);
		written = snprintf(data + offset, data_len - offset, "%s", line_data);

		if (written < 0) {
			free(data);
			data = NULL;
			break;
		}

		offset += written;
	}

	return data;
}

int multistring_get_lines(struct multistring *mstr)
{
	if (!mstr) {
		return -EINVAL;
	}

	return mstr->num_strings;
}

int multistring_truncate(struct multistring *mstr, int line, int col)
{
	int err;
	int l;

	if (!mstr) {
		return -EINVAL;
	}

	if (line < 0 || line > mstr->num_strings ||
	    col < 0 || col > multistring_line_get_length(mstr, line)) {
		return -ERANGE;
	}

	if ((err = multistring_line_truncate(mstr, line, col)) < 0) {
		return err;
	}

	for (l = line + 1; l < mstr->max_strings; l++) {
		if (mstr->strings[l]) {
			string_free(&mstr->strings[l]);
			mstr->num_strings--;
		}
	}

	return 0;
}
