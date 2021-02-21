#ifndef BUFFER_H
#define BUFFER_H

#include "telex.h"

struct buffer;
struct snippet;
struct line;

int buffer_open(struct buffer **buffer, const char *path);
int buffer_close(struct buffer **buffer);
int buffer_save(struct buffer *buffer);
int buffer_append(struct buffer *buffer, char chr);
const char* buffer_get_data(struct buffer *buffer);

int buffer_clone(struct buffer *src, struct buffer **dst);

int buffer_get_line_at(struct buffer *buffer, const char *pos);
int buffer_get_snippet(struct buffer *buffer, const int start, const int lines,
		       struct snippet **snippet);
int buffer_get_snippet_telex(struct buffer *buffer, struct telex *start, struct telex *end,
			     struct snippet **snippet);

int          line_new(struct line **line, int no, const char *str);
int          line_free(struct line**);
int          line_get_number(struct line*);
int          line_get_length(struct line*);
const char*  line_get_data(struct line*);
struct line* line_get_next(struct line*);

int snippet_new(struct snippet**);
int snippet_new_from_string(struct snippet **snippet, const char *str,
			    const size_t len, const int first_line);
int snippet_free(struct snippet**);

int snippet_append_line(struct snippet*, struct line*);
struct line* snippet_get_first_line(struct snippet*);

#endif /* BUFFER_H */
