#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FILE_DEFAULT_MODE 0600

struct config {
	int file_default_mode;
};

#ifndef __E_CONFIG
extern struct config config;
#endif

#endif /* CONFIG_H */
