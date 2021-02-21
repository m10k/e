#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "file.h"
#include "config.h"

struct file {
	int fd;
	char *path;
	int refs;
};

static int _file_alloc(struct file **file)
{
	struct file *f;

	if(!file) {
		return(-EINVAL);
	}

	f = malloc(sizeof(*f));

	if(!f) {
		return(-ENOMEM);
	}

	memset(f, 0, sizeof(*f));
	f->fd = -1;
	f->refs = 1;

	*file = f;

	return(0);
}

static int _file_free(struct file **file)
{
	if(!file || !*file) {
		return(-EINVAL);
	}

	if(--(*file)->refs == 0) {
		if((*file)->path) {
			free((*file)->path);
		}

		memset(*file, 0, sizeof(**file));
		free(*file);
		*file = NULL;
	}

	return(0);
}

static int _file_set_path(struct file *file, const char *path)
{
	size_t len;

	if(!file || !path) {
		return(-EINVAL);
	}

	if(file->path) {
		free(file->path);
	}

	len = strlen(path) + 1;
	file->path = malloc(len);

	if(!file->path) {
		return(-ENOMEM);
	}

	snprintf(file->path, len, "%s", path);
	return(0);
}

static int _file_open_path(struct file *file, const char *path)
{
	if(!file || !path) {
		return(-EINVAL);
	}

	if(file->fd >= 0) {
		return(-EALREADY);
	}

	if(_file_set_path(file, path) < 0) {
		return(-ENOMEM);
	}

	file->fd = open(path, O_RDWR | O_CREAT,
			config.file_default_mode);

	if(file->fd < 0) {
		return(-errno);
	}

	return(0);
}

int file_open(struct file **file, const char *path)
{
	struct file *f;
	int err;

	if(_file_alloc(&f) < 0) {
		return(-ENOMEM);
	}

	err = _file_open_path(f, path);

	if(err < 0) {
		_file_free(&f);
	} else {
		*file = f;
	}

	return(err);
}

int file_close(struct file **file)
{
	if(!file || !*file) {
		return(-EINVAL);
	}

	if((*file)->fd >= 0) {
		if(close((*file)->fd) < 0) {
			perror("close");
		}

		(*file)->fd = -1;
	}

	_file_free(file);

	return(0);
}

int file_get_size(struct file *file, size_t *size)
{
	off_t prev_pos;
	off_t end;

	if(!file || !size) {
		return(-EINVAL);
	}

	if(file->fd < 0) {
		return(-EBADFD);
	}

	prev_pos = lseek(file->fd, 0, SEEK_CUR);

	if(prev_pos < 0) {
		return(-errno);
	}

	end = lseek(file->fd, 0, SEEK_END);

	if(end < 0) {
		return(-errno);
	}

	if(lseek(file->fd, prev_pos, SEEK_SET) < 0) {
		/* also, we borked the internal state */
		return(-errno);
	}

	*size = (size_t)end;
	return(0);
}

int file_read(struct file *file, char **dst, size_t *size)
{
	size_t file_size;
	off_t prev_pos;
	char *data;
	int readerr;
	int err;

	if(!file || !dst) {
		return(-EINVAL);
	}

	if(file->fd < 0) {
		return(-EBADFD);
	}

	if(file_get_size(file, &file_size) < 0) {
		return(-EIO);
	}

	prev_pos = lseek(file->fd, 0, SEEK_CUR);

	if(prev_pos < 0) {
		return(-errno);
	}

	if(lseek(file->fd, 0, SEEK_SET) < 0) {
		return(-errno);
	}

	data = malloc(file_size + 1);

	if(!data) {
		return(-ENOMEM);
	}

	err = 0;
	readerr = 0;

        if(read(file->fd, data, file_size) < 0) {
		readerr = -errno;
	} else {
		data[file_size] = 0;
	}

	if(lseek(file->fd, prev_pos, SEEK_SET) < 0) {
		/* again, the internal state is borked */
		err = -errno;
	}

	if(err < 0 || readerr < 0) {
		/*
		 * The function should only return a buffer if everything succeeded,
		 * i.e. we have read the data and the file is in the previous state.
		 * But if both the read() and lseek() fail, we return the first error
		 * to the caller.
		 */
		free(data);

		if(readerr < 0) {
			err = readerr;
		}
	} else {
		*dst = data;

		if(size) {
			*size = file_size;
		}
	}

	return(err);
}

int file_write(struct file *file, const char *data)
{
	off_t prev_pos;
	int err;

	err = 0;

	if(!file || !data) {
		return(-EINVAL);
	}

	if(file->fd < 0) {
		return(-EBADFD);
	}

	prev_pos = lseek(file->fd, 0, SEEK_CUR);

	if(prev_pos < 0) {
		return(-errno);
	}

	if(lseek(file->fd, 0, SEEK_SET) < 0) {
		return(-errno);
	}

	if(write(file->fd, data, strlen(data)) < 0) {
		err = -errno;
	}

	if(lseek(file->fd, prev_pos, SEEK_SET) < 0) {
		if(!err) {
			err = -errno;
		}
	}

	return(err);
}

int file_ref(struct file *file)
{
	if(!file) {
		return(-EINVAL);
	}

	file->refs++;
	return(0);
}
