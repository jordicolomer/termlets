#ifndef CONFIG_H
#define CONFIG_H

typedef enum {
    ACTION_NONE,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_MODE,
    ACTION_BACKSPACE,
    ACTION_ENTER,
    ACTION_START_OF_LINE,
    ACTION_END_OF_LINE,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_PAGE_UP,
    ACTION_PAGE_DOWN,
    ACTION_FIRST_LINE,
    ACTION_LAST_LINE,
    ACTION_START_SELECTION,
    ACTION_COPY,
    ACTION_PASTE,
    ACTION_CUT,
    ACTION_SAVE,
    ACTION_RELOAD,
	ACTION_INSERT,
	ACTION_SEARCH,
	ACTION_INSERT_SEMICOLON,
} Action;

extern Action * mapping;
extern char *config_file;

void load_mappings();
Action * get_mapping();

#endif
