#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FILE_DEFAULT_MODE 0600
#define CONFIG_DEFAULT_TAB_WIDTH 8

struct config {
	int file_default_mode;
	int tab_width;
};

#ifndef __E_CONFIG
extern struct config config;
#endif

#endif /* CONFIG_H */
