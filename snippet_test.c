#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "file.h"
#include "telex.h"
#include "buffer.h"

static int get_file(const char *path, char **dst, size_t *size)
{
	struct file *file;
        char *data;
        size_t data_size;
        int err;

	if(!path || !dst || !size) {
		return(-EINVAL);
	}

        err = file_open(&file, path);

        if(err < 0) {
		return(err);
        }

        err = file_read(file, &data, &data_size);

        file_close(&file);

        if(err < 0) {
		return(err);
        }

	*dst = data;
	*size = data_size;

	return(0);
}

int test(const char *path, const char *start_expr, const char *end_expr)
{
	struct snippet *snip;
	struct line *line;
        struct telex *start_telex;
	struct telex *end_telex;
	const char *start_ptr;
	const char *end_ptr;
	const char *err_ptr;
	char *data;
	size_t data_size;
	int err;

	err = get_file(path, &data, &data_size);

	if(err < 0) {
		return(err);
	}

        err = telex_parse(start_expr, &start_telex, &err_ptr);

        if(err < 0) {
                printf("Telex parse error near `%s'\n", err_ptr);
                return(err);
        }

	err = telex_parse(end_expr, &end_telex, &err_ptr);

	if(err < 0) {
		printf("Telex parse error near `%s'\n", err_ptr);
		return(err);
	}

        start_ptr = telex_lookup(start_telex, data, data_size, data);

	if(!start_ptr) {
		printf("Expression `%s' didn't yield any results\n", start_expr);
		return(-ENOENT);
	}

	end_ptr = telex_lookup(end_telex, data, data_size, start_ptr);

	if(!end_ptr) {
		printf("Expression `%s' didn't yield any results\n", end_expr);
		return(-ENOENT);
	}

	err = snippet_new_from_string(&snip, start_ptr, end_ptr - start_ptr, 1);

	if(err < 0) {
		fprintf(stderr, "snippet_new_from_string: %s\n", strerror(-err));
		return(err);
	}

	for(line = snippet_get_first_line(snip); line; line = line_get_next(line)) {
		printf("%04d: %s\n", line_get_number(line), line_get_data(line));
	}

	snippet_free(&snip);

        return(0);
}

int main(int argc, char *argv[])
{
        const char *file;
        const char *start;
	const char *end;

        if(argc < 3) {
                printf("Usage: %s file start end\n", argv[0]);
                return(1);
        }

        file = argv[1];
        start = argv[2];
	end = argv[3];

        return(test(file, start, end));
}
