#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "buffer.h"
#include "file.h"
#include "config.h"
#include <telex/telex.h>

struct buffer {
	struct file *file;
	char *data;
	size_t size;
};

struct line {
	struct line *next;
	int no;
	char *data;
	size_t len;
	const char *source;
};

struct snippet {
	struct line *first_line;
	struct line *last_line;
	size_t lines;

	struct {
		const char *pos;
		struct line *line;
	} sel_start, sel_end;
};

static int _buffer_new(struct buffer **buffer)
{
	struct buffer *buf;

	if(!buffer) {
		return(-EINVAL);
	}

	buf = malloc(sizeof(*buf));

	if(!buf) {
		return(-ENOMEM);
	}

	memset(buf, 0, sizeof(*buf));
	*buffer = buf;

	return(0);
}

static int _buffer_free(struct buffer **buffer)
{
	if(!buffer || !*buffer) {
		return(-EINVAL);
	}

	if((*buffer)->data) {
		free((*buffer)->data);
	}

	if((*buffer)->file) {
		file_close(&((*buffer)->file));
	}

	memset(*buffer, 0, sizeof(**buffer));
	free(*buffer);
	*buffer = NULL;

	return(0);
}

int buffer_clone(struct buffer *src, struct buffer **dst)
{
	struct buffer *nbuf;
	int err;

	err = _buffer_new(&nbuf);

	if(err < 0) {
		return(err);
	}

	nbuf->data = malloc(src->size);

	if(!nbuf->data) {
		_buffer_free(&nbuf);
		return(-ENOMEM);
	}

	memcpy(nbuf->data, src->data, src->size);
	nbuf->size = src->size;

	err = file_ref(src->file);

	if(err < 0) {
		_buffer_free(&nbuf);
		return(err);
	}

	nbuf->file = src->file;
	*dst = nbuf;

	return(0);
}

int buffer_open(struct buffer **buffer, const char *path, const int readonly)
{
	struct buffer *buf;
	char *data;
	int err;

	if(_buffer_new(&buf) < 0) {
		return(-ENOMEM);
	}

	err = file_open(&(buf->file), path, readonly);

	if(err < 0) {
		_buffer_free(&buf);
		return(err);
	}

	err = file_read(buf->file, &data, &(buf->size));

	if(err < 0) {
		_buffer_free(&buf);
		return(err);
	}

#ifdef DEBUG
	fprintf(stderr, "Read %lu bytes from %s\n", strlen(data), path);
#endif /* DEBUG */

	buf->data = data;
	*buffer = buf;

	return(err);
}

int buffer_close(struct buffer **buffer)
{
	if(!buffer || !*buffer) {
		return(-EINVAL);
	}

        return(_buffer_free(buffer));
}

int buffer_save(struct buffer *buffer)
{
        if(!buffer) {
		return(-EINVAL);
	}

	return(file_write(buffer->file, buffer->data));
}

int buffer_append(struct buffer *buffer, char chr)
{
	char *new_data;
	size_t new_size;

	if(!buffer) {
		return(-EINVAL);
	}

	new_size = buffer->size + 1;
	new_data = malloc(new_size);

	if(!new_data) {
		return(-ENOMEM);
	}

	memcpy(new_data, buffer->data, buffer->size);
	new_data[buffer->size] = chr;

	free(buffer->data);

	buffer->data = new_data;
	buffer->size = new_size;

	return(0);
}

const char* buffer_get_data(struct buffer *buffer)
{
	return(buffer->data);
}

size_t buffer_get_size(struct buffer *buffer)
{
	return(buffer->size);
}

static int _get_snippet_extent(struct buffer *buffer, const int start, const int lines,
			       const char **snip_start, const char **snip_end)
{
	struct telex_error *errors;
	struct telex *start_telex;
	struct telex *end_telex;
	const char *start_ptr;
	const char *end_ptr;
	char telex_str[16];
	int error;

	if(!buffer || !snip_start || !snip_end) {
		return(-EINVAL);
	}

        error = 0;
	start_telex = NULL;
	end_telex = NULL;

	snprintf(telex_str, sizeof(telex_str), ":%d", start);
	if (telex_parse(&start_telex, telex_str, &errors) < 0) {
		fprintf(stderr, "Could not parse telex \"%s\"\n", telex_str);
		error = -EINVAL;
		goto cleanup;
	}

	snprintf(telex_str, sizeof(telex_str), ":%d", start + lines);
	if (telex_parse(&end_telex, telex_str, &errors) < 0) {
		fprintf(stderr, "Could not parse telex \"%s\"\n", telex_str);
		error = -EINVAL;
		goto cleanup;
	}

	if (!(start_ptr = telex_lookup(start_telex, buffer->data, buffer->size, buffer->data))) {
		fprintf(stderr, "Could not lookup start\n");
		error = -ERANGE;
		goto cleanup;
	}

	if (!(end_ptr = telex_lookup(end_telex, buffer->data, buffer->size, buffer->data))) {
		/*
		 * The specified line is behind the end of the buffer, so we
		 * limit the snippet to the end of the buffer.
		 */
		end_ptr = buffer->data + buffer->size;
	}

cleanup:
	if (error == 0) {
		*snip_start = start_ptr;
		*snip_end = end_ptr;
	}

	if (start_telex) {
		telex_free(&start_telex);
	}
	if (end_telex) {
		telex_free(&end_telex);
	}

	return error;
}

int snippet_new(struct snippet **snippet)
{
	struct snippet *snip;

	if(!snippet) {
		return(-EINVAL);
	}

	snip = malloc(sizeof(*snip));

	if(!snip) {
		return(-ENOMEM);
	}

	memset(snip, 0, sizeof(*snip));
	*snippet = snip;

	return(0);
}

int snippet_append_line(struct snippet *snippet, struct line *line)
{
	if(!snippet || !line) {
		return(-EINVAL);
	}

	if(!snippet->first_line) {
		snippet->first_line = line;
	} else {
		snippet->last_line->next = line;
	}

	snippet->last_line = line;
	snippet->lines++;
	line->next = NULL;

	return(0);
}

struct line* snippet_get_first_line(struct snippet *snippet)
{
	return(snippet ? snippet->first_line : NULL);
}

int snippet_new_from_string(struct snippet **snippet, const char *str,
			    const size_t len, const int first_line,
			    const char *sel_start, const char *sel_end)
{
	struct snippet *snip;
	const char *pos;
	int cur_line;

	if(!snippet || !str) {
		return(-EINVAL);
	}

	if(snippet_new(&snip) < 0) {
		return(-ENOMEM);
	}

	pos = str;
	cur_line = first_line;

	while(pos < str + len && *pos) {
		struct line *line;
		const char *line_start;
		int line_len;

		if(line_new(&line, cur_line, pos) < 0) {
			break;
		}

		line_len = line_get_length(line);
		line_start = line_get_data(line);

		if(sel_start >= pos && sel_start < pos + line_len) {
			snippet_set_selection_start(snip, line, sel_start - pos + line_start);
		}

		if(sel_end >= pos && sel_end < pos + line_len) {
			snippet_set_selection_end(snip, line, sel_end - pos + line_start);
		}

		if(snippet_append_line(snip, line) < 0) {
			line_free(&line);
			break;
		}

		pos += line_len;
		cur_line++;
	}

	*snippet = snip;
	return(0);
}

int snippet_free(struct snippet **snippet)
{
	struct line *line;

	if(!snippet || !*snippet) {
		return(-EINVAL);
	}

	while((*snippet)->first_line) {
		line = (*snippet)->first_line;
		(*snippet)->first_line = line->next;

		line_free(&line);
	}

	free(*snippet);
	*snippet = NULL;

	return(0);
}

int buffer_get_snippet(struct buffer *buffer, const int start, const int lines,
		       const char *sel_start, const char *sel_end,
		       struct snippet **snippet)
{
	struct snippet *snip;
	const char *snip_start;
	const char *snip_end;
	int err;

	snip_start = NULL;
	snip_end = NULL;

	err = _get_snippet_extent(buffer, start, lines, &snip_start, &snip_end);
	if(err < 0) {
		return(err);
	}
	err = snippet_new_from_string(&snip, snip_start, snip_end - snip_start, start,
				      sel_start, sel_end);
	if(err < 0) {
		return(err);
	}

	*snippet = snip;

	return(0);
}

int buffer_get_line_at(struct buffer *buffer, const char *pos)
{
	const char *cur;
	int line;

	if(pos < buffer->data || pos > buffer->data + buffer->size) {
		return(-ERANGE);
	}

	for(line = 1, cur = buffer->data; *cur && cur < pos; cur++) {
		if(*cur == '\n') {
			line++;
		}
	}

	return(line);
}

int buffer_get_col_at(struct buffer *buffer, const char *pos)
{
	const char *cur;
	int col;

	if(pos < buffer->data || pos > buffer->data + buffer->size) {
		return(-ERANGE);
	}

	for(col = 0, cur = pos; cur >= buffer->data && *cur != '\n'; cur--) {
		col++;
	}

	return(col);
}

int snippet_set_selection_start(struct snippet *snip, struct line *line, const char *start)
{
	if(!snip) {
		return(-EINVAL);
	}

	snip->sel_start.pos = start;
	snip->sel_start.line = line;

	return(0);
}

int snippet_set_selection_end(struct snippet *snip, struct line *line, const char *end)
{
	if(!snip) {
		return(-EINVAL);
	}

	snip->sel_end.pos = end;
	snip->sel_end.line = line;

	return(0);
}

const char *snippet_get_selection_start(struct snippet *snip)
{
	if(!snip) {
		return(NULL);
	}

	return(snip->sel_start.pos);
}

const char *snippet_get_selection_end(struct snippet *snip)
{
	if(!snip) {
		return(NULL);
	}

	return(snip->sel_end.pos);
}

int buffer_get_snippet_telex(struct buffer *buffer, struct telex *start, struct telex *end,
			     const int lines, struct snippet **snippet)
{
	const char *start_pos;
	const char *end_pos;
	int start_line;
	int end_line;
	int center_line;
	int err;

	/* end may be NULL */
	if(!buffer || !start || !snippet) {
		return(-EINVAL);
	}

	end_pos = NULL;
	start_pos = telex_lookup(start, buffer->data, buffer->size, buffer->data);

	if(!start_pos) {
		return(-ERANGE);
	}

	if(end) {
		end_pos = telex_lookup(end, buffer->data, buffer->size, telex_is_relative(end) ? start_pos : buffer->data);

		if(end_pos < start_pos) {
			const char *swap;

			swap = end_pos;
			end_pos = start_pos;
			start_pos = swap;
		}
	}

	if(!end_pos) {
		end_pos = start_pos + 1;
		end_line = buffer_get_line_at(buffer, end_pos);
	} else {
		end_line = buffer_get_line_at(buffer, end_pos);

		if(start_pos == end_pos) {
			end_pos++;
		}
	}
	start_line = buffer_get_line_at(buffer, start_pos);
	center_line = start_line + (end_line - start_line) / 2;

	if(center_line - lines / 2 < start_line) {
		start_line = center_line - lines / 2;
		end_line = start_line + lines;
	} else if(end_line - start_line > lines) {
		end_line = start_line + lines;
	}

	if(start_line <= 0) {
		start_line = 1;
		end_line = lines;
	}

	if(start_line <= 0 || end_line <= 0) {
		return(-ERANGE);
	}

	err = buffer_get_snippet(buffer, start_line, end_line - start_line + 1,
				 start_pos, end_pos, snippet);

	if(err < 0) {
		return(err);
	}

	return(0);
}

static int _linelen(const char *str)
{
	int n;

	if(!str) {
		return(-EINVAL);
	}

	n = 0;

	while(*str) {
		n++;

		if(*str == '\n') {
			break;
		}

		str++;
	}

	return(n);
}

int line_new(struct line **line, int no, const char *str)
{
	struct line *l;
	int len;

	if(!line || !str) {
		return(-EINVAL);
	}

	l = malloc(sizeof(*l));

	if(!l) {
		return(-ENOMEM);
	}

	len = _linelen(str);
	l->data = malloc(len + 1);

	if(!l->data) {
		free(l);
		return(-ENOMEM);
	}

	memcpy(l->data, str, len);
	l->data[len] = 0;
	l->no = no;
	l->len = len;
	l->source = str;

	*line = l;

	return(0);
}

int line_free(struct line **line)
{
	if(!line || !*line) {
		return(-EINVAL);
	}

	if((*line)->data) {
		free((*line)->data);
	}
	free(*line);

	*line = NULL;
	return(0);
}

int line_get_number(struct line *line)
{
	if(!line) {
		return(-EINVAL);
	}

	return(line->no);
}

const char* line_get_data(struct line *line)
{
	if(!line) {
		return(NULL);
	}

	return(line->data);
}

int line_get_length(struct line *line)
{
	if(!line) {
		return(-EINVAL);
	}

	return(line->len);
}

struct line* line_get_next(struct line *line)
{
	return(line ? line->next : NULL);
}

static int _buffer_lookup_telex(struct buffer *buffer, struct telex *telex, const char **result)
{
	const char *location;

	if (!buffer || !telex || !result) {
		return -EINVAL;
	}

	location = telex_lookup(telex, buffer->data, buffer->size, buffer->data);

	if (!location) {
		return -ENOENT;
	}

	*result = location;
	return 0;
}

static int _buffer_lookup_start_end(struct buffer *buffer,
				    struct telex *start_telex, struct telex *end_telex,
				    const char **start, const char **end)
{
	const char *buffer_start;
	size_t buffer_size;
	const char *start_ptr;
	const char *end_ptr;

	if (!buffer || !start || !start_telex || !start || !end) {
		return -EINVAL;
	}

	buffer_start = buffer_get_data(buffer);
	buffer_size = buffer_get_size(buffer);

	if (!(start_ptr = telex_lookup(start_telex, buffer_start, buffer_size, buffer_start))) {
		return -ERANGE;
	}

	/*
	 * If `end_telex' is NULL, we assume the user wants everything to the end of the buffer,
	 * otherwise we assume that the user intends `end_telex' to be a valid expression.
	 */
	if (!end_telex) {
		end_ptr = buffer_start + buffer_size;
	} else if (!(end_ptr = telex_lookup(end_telex, buffer_start, buffer_size,
					    telex_is_relative(end_telex) ? start_ptr : buffer_start))) {
		return -ERANGE;
	}

	/* The second expression does not necessarily evaluate to a location behind the start */
	if (start_ptr < end_ptr) {
		*start = start_ptr;
		*end = end_ptr;
	} else {
		*start = end_ptr;
		*end = start_ptr;
	}

	return 0;
}

int buffer_get_substring(struct buffer *buffer, struct telex *src_start, struct telex *src_end,
			 const char **substring_dst, size_t *substring_size_dst)
{
	const char *src_start_ptr;
	const char *src_end_ptr;
	ssize_t required_size;
	ssize_t substring_size;
	char *substring;
        int error;

	if (!buffer || !src_start || !substring_dst) {
		return -EINVAL;
	}

	if ((error = _buffer_lookup_start_end(buffer, src_start, src_end,
					      &src_start_ptr, &src_end_ptr)) < 0) {
		return error;
	}

	required_size = (ssize_t)(src_end_ptr - src_start_ptr);
	substring_size = required_size + 1;

	if (substring_size < required_size) {
		return -ERANGE;
	}

	if (!(substring = malloc(substring_size))) {
		return -ENOMEM;
	}

	memcpy(substring, src_start_ptr, required_size);
	substring[required_size] = 0;

	*substring_dst = substring;
	*substring_size_dst = substring_size;
	return 0;
}

int buffer_insert(struct buffer *buffer, const char *insertion, struct telex *start)
{
	const char *insertion_pos;
	size_t insertion_len;
	size_t insertion_offset;
	size_t suffix_len;
	char *new_data;
	size_t new_size;

	if (_buffer_lookup_telex(buffer, start, &insertion_pos) < 0) {
		return -ERANGE;
	}

	insertion_offset = (size_t)(insertion_pos - buffer->data);
	insertion_len = strlen(insertion);
	suffix_len = buffer->size - insertion_offset;
	new_size = buffer->size + insertion_len;

	if (!(new_data = realloc(buffer->data, new_size))) {
		return -ENOMEM;
	}

	memcpy(new_data + insertion_offset + insertion_len,
	       new_data + insertion_offset,
	       suffix_len);
	memcpy(new_data + insertion_offset,
	       insertion,
	       insertion_len);

	buffer->data = new_data;
	buffer->size = new_size;

	return 0;
}

int buffer_overwrite(struct buffer *buffer, const char *insertion, struct telex *start, struct telex *end)
{
	const char *dst_start;
	const char *dst_end;
	size_t offset_start;
	size_t offset_end;
	size_t src_size;
	size_t dst_size;
	ssize_t required_space;

	/* if end was specified, overwrite only from start to end, otherwise overwrite as much as needed */

	if (_buffer_lookup_telex(buffer, start, &dst_start) < 0) {
		return -ERANGE;
	}

	if (!end) {
		dst_end = buffer->data + buffer->size;
	} else if (_buffer_lookup_telex(buffer, end, &dst_end) < 0) {
		return -ERANGE;
	}

	offset_start = (size_t)(dst_start - buffer->data);
	offset_end = (size_t)(dst_end - buffer->data);
	src_size = strlen(insertion);
	dst_size = (size_t)(dst_end - dst_start);
	required_space = src_size - dst_size;

	if (required_space > 0) {
		char *new_data;

		if (!(new_data = realloc(buffer->data, buffer->size + required_space))) {
			return -ENOMEM;
		}

		if (end) {
			memmove(new_data + offset_end + required_space,
				new_data + offset_end,
				buffer->size - offset_end);
		}

		buffer->data = new_data;
		buffer->size += required_space;
	}

	memcpy(buffer->data + offset_start, insertion, src_size);
	return 0;
}
