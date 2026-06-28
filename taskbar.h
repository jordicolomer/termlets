#include "window.h"

extern int should_quit;

Window * TaskBar_new();
void TaskBar_switch(Window *w);
void cycle_task();
void vterminal_new();
void file_manager_new();
void file_editor_new();
void TaskBar_switch_frame(Window *w);
