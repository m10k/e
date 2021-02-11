#ifndef BUFFER_H
#define BUFFER_H

struct buffer;

int buffer_open(struct buffer **buffer, const char *path);
int buffer_close(struct buffer **buffer);
int buffer_save(struct buffer *buffer);
int buffer_append(struct buffer *buffer, char chr);
const char* buffer_get_data(struct buffer *buffer);

#endif /* BUFFER_H */
