#ifndef E_EDITOR_H
#define E_EDITOR_H

struct editor;

int editor_new(struct editor **editor);
int editor_open(struct editor *editor, const char *path, const int readonly);
int editor_run(struct editor *editor);
int editor_quit(struct editor *editor);
int editor_free(struct editor **editor);
int editor_set_var(struct editor *editor, const char *var, const char *data);
int editor_get_var(struct editor *editor, const char *var, const char **data);

#endif /* E_EDITOR_H */
