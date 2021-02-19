int test(const char *path, const char *cmd,
         const char *arg0, const char *arg1)
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

        err = telex_parse(arg0, &telex, &ptr);

        if(err < 0) {
                printf("Telex parse error near `%s'\n", ptr);
                return(err);
        }

        ptr = telex_lookup(telex, data, data_size, data);

        if(ptr) {
                printf("[%s]: %s\n", arg0, ptr);
        } else {
                printf("No match\n");
        }

        return(0);
}

int main(int argc, char *argv[])
{
        const char *in;
        const char *cmd;
        const char *arg0;
        const char *arg1;

        if(argc < 5) {
                printf("Usage: %s file cmd arg0 arg1\n", argv[0]);
                return(1);
        }

        in = argv[1];
        cmd = argv[2];
        arg0 = argv[3];
        arg1 = argv[4];

        return(test(in, cmd, arg0, arg1));
}
/*
                struct telex *exprs;
                int num_exprs;

                exprs = NULL;

                num_exprs = telex_parse(argv[i], &exprs);

                if(num_exprs < 0) {
                        printf("telex_parse: %s\n", strerror(-num_exprs));
                        continue;
                }

                if(num_exprs > 0) {
                        telex_dbg(exprs, 0);
                }
        }

        return(0);
}
*/
