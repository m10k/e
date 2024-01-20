#ifndef E_STRING_H
#define E_STRING_H

struct string;

int string_new(struct string**);
int string_free(struct string**);

struct string* string_strdup(const char *s);
struct string* string_strndup(const char *s, size_t n);
struct string* string_get_substring(struct string *str, int start, int length);

int string_insert_char(struct string *str, const int pos,
		       const char chr);
int string_insert_string(struct string *dst, const int pos,
			 struct string *src);
int string_remove_char(struct string *str, const int pos);
int string_get_length(struct string *str);
int string_truncate(struct string *str, const int pos);
int string_clone(struct string *src, struct string **dst);
const char* string_get_data(struct string *str);

#endif /* E_STRING_H */
