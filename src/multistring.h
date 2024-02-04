#ifndef E_MULTISTRING_H
#define E_MULTISTRING_H

struct multistring;

int multistring_new(struct multistring **dst);
int multistring_free(struct multistring **mstr);

int multistring_get_length(struct multistring *mstr);
char* multistring_get_data(struct multistring *mstr);
int multistring_get_lines(struct multistring *mstr);
int multistring_truncate(struct multistring *mstr, int line, int col);

int multistring_line_remove(struct multistring *mstr, int line);
int multistring_line_get_length(struct multistring *mstr, int line);
int multistring_line_insert_char(struct multistring *mstr, int line, int col, char chr);
int multistring_line_remove_char(struct multistring *mstr, int line, int col);
int multistring_line_truncate(struct multistring *mstr, int line, int col);
const char* multistring_line_get_data(struct multistring *mstr, int line);

#endif /* E_MULTISTRING_H */
