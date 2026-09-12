#ifndef CONFIG_H
#define CONFIG_H

#define ACTIONS(X) \
    X(NONE) \
    X(LEFT) \
    X(RIGHT) \
    X(NEXT_WORD) \
    X(PREV_WORD) \
    X(SWITCH_MODE) \
    X(BACKSPACE) \
    X(ENTER) \
    X(START_OF_LINE) \
    X(END_OF_LINE) \
    X(UP) \
    X(DOWN) \
    X(PAGE_UP) \
    X(PAGE_DOWN) \
    X(FIRST_LINE) \
    X(LAST_LINE) \
    X(START_SELECTION) \
    X(COPY) \
    X(PASTE) \
    X(CUT) \
    X(SAVE) \
    X(RELOAD) \
    X(INSERT) \
    X(SEARCH) \
    X(INSERT_SEMICOLON) \
    X(WINDOW_MANAGER) \
    X(FILE_MANAGER) \
    X(TERMINAL) \
    X(PREVIOUS_TAB) \
    X(PARENT_DIRECTORY) \
    X(RENAME) \
    X(EDIT) \
    X(NEXT_TAB) \
    X(NEXT_WINDOW)

#define MAKE_ENUM(name) ACTION_##name,

typedef enum {
    ACTIONS(MAKE_ENUM)
} Action;

#undef MAKE_ENUM

#define MAKE_ACTION_STRING(name) [ACTION_##name] = #name,

static const char *action_names[] = {
    ACTIONS(MAKE_ACTION_STRING)
};

#undef MAKE_ACTION_STRING

typedef enum { WT_NONE, WT_EDITOR, WT_FILE_MANAGER, WT_TABS, WT_WINDOW_MANAGER, WT_TERMINAL } WindowType;

extern Action * mapping;
extern Action * mapping_edit;
extern char *config_file;

void load_mappings();
Action * get_mapping();
Action get_action(char c, WindowType window_type);

#endif
