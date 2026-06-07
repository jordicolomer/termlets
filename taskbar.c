#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taskbar.h"
#include "file_manager.h"
#include "terminal.h"

int window_x = 5;
int window_y = 3;

// start menu

void start_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1 - startMenu->hidden;
  Window_bring_to_bottom(startMenu);
  open_menu = startMenu;
}

Window *taskBar;
Window *selectedTask = NULL;
int x = 11;
int unselectedTaskColor = 251;
int selectedTaskColor = 15;

void task_on_mouse_down(struct Window *w, int x, int y){
    if (selectedTask != NULL) selectedTask->bg = unselectedTaskColor;

    Window *task = w->data;
    focused = task;
    Window_bring_to_bottom(task);

    w->bg = selectedTaskColor;
    selectedTask = w;
}

Window * TaskBar_new_task(const char * name, Window *fm){
  if (selectedTask != NULL) selectedTask->bg = unselectedTaskColor;
  Window *task = Window_add_widget(taskBar, x, -1, -1, 0, 9, 1, name, 0, selectedTaskColor);
  selectedTask = task;

  task->data = fm;
  task->on_mouse_down = task_on_mouse_down;
  x += strlen(name)-1;

  return task;
}

void TaskBar_switch(Window *w){
  if (w == NULL) return;
  if (selectedTask != NULL) selectedTask->bg = unselectedTaskColor;
  w->bg = selectedTaskColor;
  selectedTask = w;
}

void file_manager_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1;
  open_menu = NULL;

  Window *fm = FileExplorer_new(window_x, -1, window_y, -1, 80, 30);
  window_x += 10;
  window_y += 3;
  fm->parent = root;
  fm->id = "FileExplorer";
  focused = fm;
  Window_append(root, fm);

  Window *task = TaskBar_new_task("📁 Explorer", fm);
  fm->data = task;
}

void terminal_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1;
  open_menu = NULL;

  Window *fm = Terminal_new(window_x, -1, window_y, -1, 80, 30);
  window_x += 10;
  window_y += 3;
  fm->parent = root;
  fm->id = "Terminal";
  focused = fm;
  Window_append(root, fm);

  Window *task = TaskBar_new_task("💻 Terminal", fm);
  fm->data = task;

}

Window * TaskBar_new(){
  taskBar = malloc(sizeof *taskBar);
  Window_init(taskBar, 0, -1, -1, 0, -1, 1);
  Window_append(root, taskBar);
  int taskbar_color = 255;
  taskbar_color = 103;

  Window *start = Window_add_widget(taskBar, 0, -1, -1, 0, 9, 1, "💻 Start", 0, taskbar_color);
  start->on_mouse_down = start_mouse_down;

  Window *startMenu = malloc(sizeof *startMenu);
  Window_init(startMenu, 0, -1, -1, 1, 10, 2);
  startMenu->hidden = 1;
  Window_append(root, startMenu);
  startMenu->id = "menu";
  start->data = startMenu;

  Window *file_manager = Window_add_widget(startMenu, 0, -1, 0, -1, 16, 1, "📁 File Manager", 0, taskbar_color);
  file_manager->on_mouse_down = file_manager_mouse_down;
  file_manager->data = startMenu;

  Window *terminal = Window_add_widget(startMenu, 0, -1, 1, -1, 16, 1, "💻 Terminal", 0, taskbar_color);
  terminal->on_mouse_down = terminal_mouse_down;
  terminal->data = startMenu;

  //Window *tasks = Window_add_widget(taskBar, 20, -1, -1, 0, 9, 1, , 0, 105);
  
}

