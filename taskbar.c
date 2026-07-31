#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taskbar.h"
#include "file_manager.h"
#include "terminal.h"
#include "vterm_terminal.h"
#include "logger.h"
#include "editor.h"
#include "chess.h"
#include "common.h"

int window_x = 5;
int window_y = 3;
int should_quit = 0;

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
//int unselectedTaskColor = 251;
int unselectedTaskColor = 242;
int selectedTaskColor = SELECTED_COLOR;


void task_on_mouse_down(struct Window *w, int x, int y){
    if (selectedTask != NULL) selectedTask->bg = unselectedTaskColor;

    Window *task = w->data;
    focused = task;
    Window_bring_to_bottom(task);

    w->bg = selectedTaskColor;
    selectedTask = w;
}

void cycle_task(){
  //LOG_INFO("cycle_task %p", selectedTask);
  if (selectedTask == NULL) return;
  Window *task = selectedTask->next;
  if (selectedTask->next == NULL) task = selectedTask->parent->head->next;
  TaskBar_switch(task);
  task_on_mouse_down(task, 0, 0);
}

Window * TaskBar_new_task(const char * name, Window *fm){
  if (selectedTask != NULL) selectedTask->bg = unselectedTaskColor;
  Window *task = Window_add_widget(taskBar, x, -1, -1, 0, 11, 1, name, 0, selectedTaskColor);
  selectedTask = task;

  task->data = fm;
  task->on_mouse_down = task_on_mouse_down;
  x += task->width+1;

  return task;
}

void TaskBar_switch(Window *w){
  if (w == NULL) return;
  if (selectedTask != NULL) selectedTask->bg = unselectedTaskColor;
  w->bg = selectedTaskColor;
  selectedTask = w;
}

void TaskBar_switch_frame(Window *w){
  Window_bring_to_bottom(w);
  TaskBar_switch((Window *)(w->data));
  focused = w;
}

void file_manager_new(){
  Window *fm = FileExplorer_new(window_x, -1, window_y, -1, 90, 30);
  window_x += 10;
  window_y += 3;
  fm->parent = root;
  fm->id = "FileExplorer";
  focused = fm;
  Window_append(root, fm);

  Window *task = TaskBar_new_task("📁 Explorer", fm);
  fm->data = task;
}

void file_manager_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1;
  open_menu = NULL;

  file_manager_new();
}

void file_editor_new(){
  Window *fm = Editor_new(window_x, -1, window_y, -1, 80, 30);
  window_x += 10;
  window_y += 3;
  fm->parent = root;
  fm->id = "FileEditor";
  focused = fm;
  Window_append(root, fm);

  Window *task = TaskBar_new_task("📝 Editor", fm);
  fm->data = task;
}

void file_editor_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1;
  open_menu = NULL;

  file_editor_new();
}


void chess_new(){
  Window *fm = Chess_new(window_x, window_y);
  window_x += 10;
  window_y += 3;
  fm->parent = root;
  fm->id = "Chess";
  focused = fm;
  Window_append(root, fm);

  Window *task = TaskBar_new_task("🏁 Chess", fm);
  fm->data = task;
}

void chess_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1;
  open_menu = NULL;

  chess_new();
}
/*
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
*/

void vterminal_new(){
  Window *fm = VTermTerminal_new(window_x, -1, window_y, -1, 80, 30);
  window_x += 10;
  window_y += 3;
  fm->parent = root;
  fm->id = "Terminal";
  focused = fm;
  Window_append(root, fm);

  Window *task = TaskBar_new_task("💻 Terminal", fm);
  fm->data = task;
}

void vterminal_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1;
  open_menu = NULL;

  vterminal_new();
}

void quit_mouse_down(struct Window *w, int x, int y){
  Window *startMenu = w->data;
  startMenu->hidden = 1;
  open_menu = NULL;
  should_quit = 1;
}

Window * TaskBar_new(){
  taskBar = malloc(sizeof *taskBar);
  Window_init(taskBar, 0, -1, -1, 0, -1, 1);
  taskBar->id = "taskBar";
  Window_append(root, taskBar);
  int taskbar_color = 255;
  taskbar_color = 103;

  Window *start = Window_add_widget(taskBar, 0, -1, -1, 0, 9, 1, "💻 Start", 0, taskbar_color);
  start->on_mouse_down = start_mouse_down;

  Window *startMenu = malloc(sizeof *startMenu);
  Window_init(startMenu, 0, -1, -1, 1, 16, 5);
  startMenu->hidden = 1;
  Window_append(root, startMenu);
  startMenu->id = "menu";
  start->data = startMenu;

  Window *chess = Window_add_widget(startMenu, 0, -1, 0, -1, 16, 1, "🏁 Chess", 0, taskbar_color);
  chess->on_mouse_down = chess_mouse_down;
  chess->data = startMenu;
  //chess->on_mouse_down = Window_execute_lambda;
  //chess->lambda = create_lambda(Chess_new, 0);

  //file_editor->on_mouse_down = file_editor_mouse_down;
  //file_editor->data = startMenu;

  Window *file_editor = Window_add_widget(startMenu, 0, -1, 1, -1, 16, 1, "📝 Editor", 0, taskbar_color);
  file_editor->on_mouse_down = file_editor_mouse_down;
  file_editor->data = startMenu;

  Window *file_manager = Window_add_widget(startMenu, 0, -1, 2, -1, 16, 1, "📁 File Manager", 0, taskbar_color);
  file_manager->on_mouse_down = file_manager_mouse_down;
  file_manager->data = startMenu;

  /*Window *terminal = Window_add_widget(startMenu, 0, -1, 1, -1, 16, 1, "💻 Terminal", 0, taskbar_color);
  terminal->on_mouse_down = terminal_mouse_down;
  terminal->data = startMenu;*/

  Window *vterminal = Window_add_widget(startMenu, 0, -1, 3, -1, 16, 1, "💻 Terminal", 0, taskbar_color);
  vterminal->on_mouse_down = vterminal_mouse_down;
  vterminal->data = startMenu;

  Window *quit = Window_add_widget(startMenu, 0, -1, 4, -1, 16, 1, "⏻  Quit", 0, taskbar_color);
  quit->on_mouse_down = quit_mouse_down;
  quit->data = startMenu;

  //Window *tasks = Window_add_widget(taskBar, 20, -1, -1, 0, 9, 1, , 0, 105);
  
}

