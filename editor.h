#ifndef E_EDITOR_H
#define E_EDITOR_H

struct editor;

int editor_new(struct editor **editor);
int editor_open(struct editor *editor, const char *path, const int readonly);
int editor_run(struct editor *editor);
int editor_free(struct editor **editor);

#endif /* E_EDITOR_H */
