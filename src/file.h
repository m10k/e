#ifndef E_FILE_H
#define E_FILE_H

struct file;

int file_open(struct file **file, const char *path, const int readonly);
int file_close(struct file **file);

int file_get_size(struct file *file, size_t *size);
int file_read(struct file *file, char **dst, size_t *size);
int file_write(struct file *file, const char *data);
int file_ref(struct file *file);

#endif /* E_FILE_H */
