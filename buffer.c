#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "buffer.h"
#include "file.h"
#include "config.h"

struct buffer {
	struct file *file;
	char *data;
	size_t size;
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

	fprintf(stderr, "Read %lu bytes from %s\n", strlen(data), path);

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
