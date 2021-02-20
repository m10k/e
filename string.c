#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define STRING_INIT_SIZE 16

struct string {
	char *data;
	size_t size;
	size_t len;
};

int string_new(struct string **dst)
{
	struct string *str;

	str = malloc(sizeof(*str));

	if(!str) {
		return(-ENOMEM);
	}

	str->data = malloc(STRING_INIT_SIZE);

	if(!str->data) {
		free(str);
		return(-ENOMEM);
	}

	str->size = STRING_INIT_SIZE;
	str->len = 0;

	memset(str->data, 0, str->size);
	*dst = str;

	return(0);
}

int string_free(struct string **str)
{
	if(!str || !*str) {
		return(-EINVAL);
	}

        if((*str)->size > 0) {
		memset((*str)->data, 0, (*str)->size);
		free((*str)->data);
	}

	memset(*str, 0, sizeof(**str));
	free(*str);

	*str = NULL;
	return(0);
}

static int _string_grow(struct string *str, const size_t n)
{
	char *new_data;
	size_t new_size;

	new_size = str->size + n + STRING_INIT_SIZE -
		n % STRING_INIT_SIZE;

	new_data = malloc(new_size);

	if(!new_data) {
		return(-ENOMEM);
	}

	memcpy(new_data, str->data, str->len);
	new_data[str->len] = 0;

	free(str->data);
	str->data = new_data;
	str->size = new_size;

	return(0);
}

int string_insert_char(struct string *str, const int pos, const char chr)
{
	size_t suffix_len;
	int real_pos;

	if(!str) {
		return(-EINVAL);
	}

	real_pos = (pos == -1) ? str->len : pos;

	if(real_pos < 0 || real_pos > str->len) {
		return(-EOVERFLOW);
	}

	if(str->len + 1 > str->size) {
		if(_string_grow(str, 1) < 0) {
			return(-ENOMEM);
		}
	}

	/* copy the NUL-byte as well */
	suffix_len = str->len - real_pos + 1;
	memmove(str->data + real_pos + 1,
		str->data + real_pos,
		suffix_len);

	str->data[real_pos] = chr;
	str->len++;

	return(0);
}

int string_remove_char(struct string *str, const int pos)
{
	size_t suffix_len;
	int real_pos;

	if(!str) {
		return(-EINVAL);
	}

	real_pos = (pos == -1) ? str->len - 1 : pos;

	if(real_pos < 0 || real_pos >= str->len) {
		return(-EOVERFLOW);
	}

	suffix_len = str->len - real_pos;
	memmove(str->data + real_pos,
		str->data + real_pos + 1,
		suffix_len);
	str->len--;

	return(0);
}

int string_truncate(struct string *str, const int pos)
{
	if(!str) {
		return(-EINVAL);
	}

	if(pos < 0 || pos > str->len) {
		return(-EOVERFLOW);
	}

	str->data[pos] = 0;
	str->len = pos;

	return(0);
}

const char* string_get_data(struct string *str)
{
	if(!str) {
		return(NULL);
	}

	return(str->data);
}

int string_get_length(struct string *str)
{
	if(!str) {
		return(-EINVAL);
	}

	return((int)str->len);
}
