#include <stdio.h>
#include "file.h"
#include "telex.h"

int test(const char *path, const char *telex_str)
{
        struct telex *telex;
        struct file *file;
        char *data;
        size_t data_size;
        int err;
        const char *ptr;

        err = file_open(&file, path);

        if(err < 0) {
                return(err);
        }

        err = file_read(file, &data, &data_size);

        file_close(&file);

        if(err < 0) {
                return(err);
        }

        err = telex_parse(telex_str, &telex, &ptr);

        if(err < 0) {
                printf("Telex parse error near `%s'\n", ptr);
                return(err);
        }

        ptr = telex_lookup(telex, data, data_size, data);

        if(ptr) {
                printf("[%s]: %s\n", telex_str, ptr);
        } else {
                printf("No match\n");
        }

        return(0);
}

int main(int argc, char *argv[])
{
        const char *file;
        const char *telex;

        if(argc < 3) {
                printf("Usage: %s file telex\n", argv[0]);
                return(1);
        }

        file = argv[1];
        telex = argv[2];

        return(test(file, telex));
}
