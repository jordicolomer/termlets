#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "config.h"

Action * mapping;


int load_mappings_from_file(void) {
    const char *home = getenv("HOME");
    if (!home) return 1;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/.config/termlets/mapping.conf", home);

    FILE *file = fopen(path, "r");
    if (!file) return 1;

    char line[256];
    char action[64];
    char value[64];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, " %63[^=] = %63s", action, value) != 2)
            continue;

        // Remove trailing whitespace from action
        char *end = action + strlen(action) - 1;
        while (end >= action && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

        int key;

        if (strlen(value) == 1) {
            // Single character: d, f, ;, s, g, etc.
            key = (unsigned char)value[0];
        } else {
            // Integer: 8, 13, etc.
            key = atoi(value);
        }

        if (strcmp(action, "LEFT") == 0) {
            mapping[(unsigned char)key] = ACTION_LEFT;
        } else if (strcmp(action, "RIGHT") == 0) {
            mapping[(unsigned char)key] = ACTION_RIGHT;
        } else if (strcmp(action, "MODE") == 0) {
            mapping[(unsigned char)key] = ACTION_MODE;
        } else if (strcmp(action, "BACKSPACE") == 0) {
            mapping[(unsigned char)key] = ACTION_BACKSPACE;
        } else if (strcmp(action, "ENTER") == 0) {
            mapping[(unsigned char)key] = ACTION_ENTER;
        } else if (strcmp(action, "START_OF_LINE") == 0) {
            mapping[(unsigned char)key] = ACTION_START_OF_LINE;
        } else if (strcmp(action, "END_OF_LINE") == 0) {
            mapping[(unsigned char)key] = ACTION_END_OF_LINE;
        } else if (strcmp(action, "UP") == 0) {
            mapping[(unsigned char)key] = ACTION_UP;
        } else if (strcmp(action, "DOWN") == 0) {
            mapping[(unsigned char)key] = ACTION_DOWN;
        } else if (strcmp(action, "PAGE_UP") == 0) {
            mapping[(unsigned char)key] = ACTION_PAGE_UP;
        } else if (strcmp(action, "PAGE_DOWN") == 0) {
            mapping[(unsigned char)key] = ACTION_PAGE_DOWN;
        } else if (strcmp(action, "FIRST_LINE") == 0) {
            mapping[(unsigned char)key] = ACTION_FIRST_LINE;
        } else if (strcmp(action, "LAST_LINE") == 0) {
            mapping[(unsigned char)key] = ACTION_LAST_LINE;
        } else if (strcmp(action, "START_SELECTION") == 0) {
            mapping[(unsigned char)key] = ACTION_START_SELECTION;
        } else if (strcmp(action, "COPY") == 0) {
            mapping[(unsigned char)key] = ACTION_COPY;
        } else if (strcmp(action, "PASTE") == 0) {
            mapping[(unsigned char)key] = ACTION_PASTE;
        } else if (strcmp(action, "CUT") == 0) {
            mapping[(unsigned char)key] = ACTION_CUT;
        } else if (strcmp(action, "SAVE") == 0) {
            mapping[(unsigned char)key] = ACTION_SAVE;
        } else if (strcmp(action, "RELOAD") == 0) {
            mapping[(unsigned char)key] = ACTION_RELOAD;
        }
    }

    fclose(file);
    return 0;
}

void load_default_mappings(){
    mapping[8] = ACTION_BACKSPACE;
    mapping[13] = ACTION_ENTER;
    mapping[';'] = ACTION_MODE;

    mapping['h'] = ACTION_LEFT;
    mapping['l'] = ACTION_RIGHT;
    mapping['j'] = ACTION_DOWN;
    mapping['k'] = ACTION_UP;

    mapping['p'] = ACTION_PAGE_DOWN;
    mapping['n'] = ACTION_PAGE_UP;

    mapping['0'] = ACTION_START_OF_LINE;
    mapping['$'] = ACTION_END_OF_LINE;

    mapping['g'] = ACTION_FIRST_LINE;
    mapping['G'] = ACTION_LAST_LINE;

    mapping['w'] = ACTION_SAVE;

    mapping[' '] = ACTION_START_SELECTION;
    mapping['c'] = ACTION_COPY;
    mapping['v'] = ACTION_PASTE;
    mapping['x'] = ACTION_CUT;
    mapping['w'] = ACTION_SAVE;
    mapping['r'] = ACTION_RELOAD;
}

void load_mappings(){
    mapping = malloc(sizeof(Action) * 256);
    memset(mapping, 0, sizeof(Action) * 256);  // Zero-initialize to prevent garbage values
    if (load_mappings_from_file()){
        load_default_mappings();
    }
}
