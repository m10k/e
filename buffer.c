#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "buffer.h"
#include "file.h"
#include "config.h"
#include "telex.h"

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
};

struct snippet {
	struct line *first_line;
	struct line *last_line;
	size_t lines;
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

int buffer_open(struct buffer **buffer, const char *path)
{
	struct buffer *buf;
	char *data;
	int err;

	if(_buffer_new(&buf) < 0) {
		return(-ENOMEM);
	}

	err = file_open(&(buf->file), path);

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

static int _get_snippet_extent(struct buffer *buffer, const int start, const int lines,
			       const char **snip_start, const char **snip_end)
{
	struct telex pos_start = {
		.next = NULL,
		.type = TELEX_LINE,
		.direction = 0,
		.data.number = start
	};
	struct telex pos_end = {
		.next = NULL,
		.type = TELEX_LINE,
		.direction = 1,
		.data.number = lines
	};

	const char *start_ptr;
	const char *end_ptr;

	if(!buffer || !snip_start || !snip_end) {
		return(-EINVAL);
	}

	start_ptr = telex_lookup(&pos_start, buffer->data, buffer->size, buffer->data);

	if(!start_ptr) {
		return(-ERANGE);
	}

	end_ptr = telex_lookup(&pos_end, buffer->data, buffer->size, start_ptr);

	if(!end_ptr) {
		/*
		 * The specified line is behind the end of the buffer, so we
		 * limit the snipped to the end of the buffer.
		 */
		end_ptr = buffer->data + buffer->size;
	}

	*snip_start = start_ptr;
	*snip_end = end_ptr;

	return(0);
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
			    const size_t len, const int first_line)
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

		if(line_new(&line, cur_line, pos) < 0) {
			break;
		}

		if(snippet_append_line(snip, line) < 0) {
			line_free(&line);
			break;
		}

		pos += line_get_length(line) + 1;
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
		       struct snippet **snippet)
{
	struct snippet *snip;
	const char *snip_start;
	const char *snip_end;
	int err;

	err = _get_snippet_extent(buffer, start, lines, &snip_start, &snip_end);

	if(err < 0) {
		return(err);
	}

	err = snippet_new_from_string(&snip, snip_start, snip_end - snip_start, start);

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

int buffer_get_snippet_telex(struct buffer *buffer, struct telex *start, struct telex *end,
			     struct snippet **snippet)
{
	const char *start_pos;
	const char *end_pos;
	int start_line;
	int end_line;

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
		end_pos = telex_lookup(end, buffer->data, buffer->size, start_pos);
	}

	if(!end_pos) {
		end_pos = buffer->data + buffer->size;
	}

	start_line = buffer_get_line_at(buffer, start_pos);
	end_line = buffer_get_line_at(buffer, end_pos);

	if(start_line <= 0 || end_line <= 0) {
		return(-ERANGE);
	}

	return(buffer_get_snippet(buffer, start_line, end_line - start_line + 1, snippet));
}

static int _linelen(const char *str)
{
	int n;

	if(!str) {
		return(-EINVAL);
	}

	n = 0;

	while(*str && *str != '\n') {
		n++;
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
