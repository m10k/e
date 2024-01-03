#ifndef BUFFER_H
#define BUFFER_H

#include <telex/telex.h>

struct buffer;
struct snippet;
struct line;

int buffer_open(struct buffer **buffer, const char *path, const int readonly);
int buffer_close(struct buffer **buffer);
int buffer_save(struct buffer *buffer);
int buffer_append(struct buffer *buffer, char chr);
const char* buffer_get_data(struct buffer *buffer);
size_t buffer_get_size(struct buffer *buffer);

int buffer_clone(struct buffer *src, struct buffer **dst);

int buffer_get_line_at(struct buffer *buffer, const char *pos);
int buffer_get_snippet(struct buffer *buffer, const int start, const int lines,
		       const char *sel_start, const char *sel_end,
		       struct snippet **snippet);
int buffer_get_snippet_telex(struct buffer *buffer, struct telex *start, struct telex *end,
			     const int lines, struct snippet **snippet);

int buffer_get_substring(struct buffer *buffer, struct telex *src_start, struct telex *src_end,
			 const char **substring, size_t *substring_length);

int buffer_insert(struct buffer *buffer, const char *insertion, struct telex *start);

int          line_new(struct line **line, int no, const char *str);
int          line_free(struct line**);
int          line_get_number(struct line*);
int          line_get_length(struct line*);
const char*  line_get_data(struct line*);
struct line* line_get_next(struct line*);

int snippet_new(struct snippet**);
int snippet_new_from_string(struct snippet **snippet, const char *str,
			    const size_t len, const int first_line,
			    const char *sel_start, const char *sel_end);
int snippet_free(struct snippet**);

int snippet_set_selection_start(struct snippet *snip, struct line *line, const char *start);
int snippet_set_selection_end(struct snippet *snip, struct line *line, const char *end);
const char *snippet_get_selection_start(struct snippet *snip);
const char *snippet_get_selection_end(struct snippet *snip);

int snippet_append_line(struct snippet*, struct line*);
struct line* snippet_get_first_line(struct snippet*);

#endif /* BUFFER_H */
