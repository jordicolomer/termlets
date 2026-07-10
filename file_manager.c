#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#include "window.h"
#include "frame.h"
#include "slider.h"
#include "file_manager.h"
#include "tabs.h"
#include "logger.h"
#include "editor.h"
#include "buffer.h"
#include "menu.h"

int ends_with(const char *str, const char *suffix)
{
  size_t len_str = strlen(str);
  size_t len_suf = strlen(suffix);

  if (len_str < len_suf)
  {
    return 0;
  }

  return strcmp(str + (len_str - len_suf), suffix) == 0;
}

void FileExplorer_select_item(ExplorerWindow * self, Window * item){
  if (item == NULL) return;
  if (self->selected != NULL){
    Window * current = self->selected->head;
    while(current != NULL){
      current->bg = 255;
      current = current->next;
    }
  }
  self->selected = item;
  //item->bg = 27;
  Window * current = item->head;
  while(current != NULL){
    current->bg = 27;
    current = current->next;
  }

  Slider_make_visible(self->slider, item);
  Slider_show_grip(self->slider);
}

void item_clicked(Window *wg, int x, int y)
{
    ExplorerWindow * self = wg->data;
    FileItemWindow *file_item = wg->data2;
    if (self->selected == file_item){
      if (file_item->is_dir){
        FileExplorer_list_files(self, file_item->path);
      }
    } else {
      FileExplorer_select_item(self, file_item);
    }
    
}

void remove_newlines(char *str) {
    char *src = str;
    char *dst = str;

    while (*src) {
        if (*src != '\n' && *src != '\r') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}



char *make_string(const char *src)
{
    size_t len = 0;
    while (src[len] != '\0') len++;

    char *copy = malloc(len + 1);
    if (!copy) return NULL;

    for (size_t i = 0; i <= len; i++) {
        copy[i] = src[i];
    }

    return copy;
}

void human_size(off_t size, char *buf, size_t buflen)
{
    const char *units[] = {"Bytes", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double s = (double)size;

    while (s >= 1024 && unit < 4) {
        s /= 1024;
        unit++;
    }

    if (unit == 0)
        snprintf(buf, buflen, "%.0f %s", s, units[unit]);
    else
        snprintf(buf, buflen, "%.1f %s", s, units[unit]);
}


// sort
#include "sort.h"
void sort_list(Window * fm, int sort_by, int reversed){
  Window * sorted = mergeSort(fm->head, sort_by, reversed);
  fm->head = sorted;
  int top = 0;
  while (sorted != NULL){
    sorted->top = top++;
    fm->tail = sorted;
    sorted = sorted->next;
  }
}

void FileExplorer_sort(ExplorerWindow * self, int sort_by){
  if (sort_by == -1) return;
  if (self->sort_by == sort_by){
    self->reversed[sort_by] = 1 - self->reversed[sort_by];
  }
  self->sort_by = sort_by;
  sort_list(self->fm, sort_by, self->reversed[sort_by]);
  for(int i =0;i<6;i++) self->sort_marker[i]->hidden = 1;
  int idx = sort_by*2+self->reversed[sort_by];
  self->sort_marker[idx]->hidden = 0;
}
// end sort

void FileExplorer_list_files(ExplorerWindow * self, char * dire){
  snprintf(self->path_label, 1024, " 📁 %s", dire);
  Window_set_id_from_path(self, dire);

  if (self->path != NULL) free(self->path);
  self->path = make_string(dire);

  LOG_INFO("FileExplorer_list_files: %p %s filename:%s", self, dire, self->win.id);
  self->selected = NULL;
  
  Window *fm = self->fm;
  Window *slider = self->slider;
  if (slider != NULL)
  {
    Slider_reset(slider);
  }
  fm->virtual_height = fm->height;

  // todo: clean properly
  fm->head = NULL;
  fm->tail = NULL;

  int j = 0;
  DIR *dir;
  struct dirent *entry;

  dir = opendir(dire); // current directory

  if (dir == NULL)
  {
    perror("opendir");
    return;
  }

  // int x = 0;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, "..") == 0) continue;
    if (strcmp(entry->d_name, ".") == 0) continue;

    char *full_path = NULL;
    int len = asprintf(&full_path, "%s/%s", dire, entry->d_name);

    FileItemWindow *file_item = malloc(sizeof *file_item);
    char *icon = "📄";
    if (entry->d_type == DT_DIR)
    {
      icon = "📁";
      file_item->is_dir = 1;
    }
    /*if (ends_with(entry->d_name, ".png"))
    {
      icon = "🖼️";
    }*/
    if (ends_with(entry->d_name, ".pdf"))
    {
      icon = "📖";
    }
    char *str = NULL;
    // memory leak here

    char * date = malloc(32);
    char * size = malloc(16);
    /*char date[32];
    date[0] = 0;
    char size[16];
    size[0] = 0;*/
    struct stat st;
    file_item->path = full_path;
    if (stat(full_path, &st) == 0){
      struct tm *tm = localtime(&st.st_mtime);
      strftime(date, 32, "%Y-%m-%d %H:%M:%S", tm);

      human_size(st.st_size, size, 16);

      file_item->size = st.st_size;
      file_item->date = st.st_mtime;

    } /*else {
      //continue;
      perror("stat");
      printf("stat failed: %s\n", strerror(errno));
    }*/
    remove_newlines(entry->d_name);


    int filename_width = 20;
    //if (fm->calculated.width != 0) filename_width = fm->calculated.width - 48;
    //LOG_INFO("fm->calculated.width: %d", fm->calculated.width);
    //len = asprintf(&str, "%s %*s %s  %10s", icon, -filename_width, entry->d_name, date, size);
    len = asprintf(&str, "%s %s", icon, entry->d_name);
    // Window_add_widget(w, fav_width, 0, j++, -1, -1, 1, str, 232, 255);

    Window_init(file_item, -1, -1, -1, -1, -1, -1);
    file_item->win.left = 0;
    file_item->win.right = 0;
    file_item->win.top = j;
    file_item->win.height = 1;
    Window_append(fm, file_item);

    Window * item = Window_add_widget(file_item, 0, -20, 0, -1, -1, 1, str, 232, 255);
    
    Window_add_widget(file_item, -33, 0, 0, -1, -1, 1, date, 232, 255);
    Window_add_widget(file_item, -12, 0, 0, -1, -1, 1, size, 232, 255);
    /*Window * item = Window_add_widget(file_item, 0, -20, 0, -1, -1, 1, str, 232, 255);
    Window_add_widget(file_item, -20, -10, 0, -1, -1, 1, date, 232, 255);
    Window_add_widget(file_item, -10, 0, 0, -1, -1, 1, size, 232, 255);*/
    //Window * date_item = Window_add_widget(fm, 0, 32, j, -1, -1, 1, date, 232, 255);
    j++;



    item->data = self;
    //item->data2 = full_path;
    item->data2 = file_item;
    item->on_mouse_down = item_clicked;

    file_item->win.data = self;
    file_item->win.data2 = full_path;

    if (self->selected == NULL) FileExplorer_select_item(self, file_item);
    //LOG_INFO("full_path: %s", full_path);
    // if (height < j) break;
    // mvwprintw(win, x++, 1, "%s %s", icon, entry->d_name);
  }
  closedir(dir);

  //sort_list(fm);
  int sort_by = self->sort_by;
  self->sort_by = -1;
  FileExplorer_sort(self, sort_by);

  // Clear remaining lines to remove previous list items
  //while (j <= self->win.calculated.height)
  //  Window_add_widget(fm, 0, 0, j++, -1, -1, 1, "", 232, 255);
}

void get_parent(char *path)
{
    if (!path) return;

    size_t len = strlen(path);
    if (len == 0) return;

    // Remove trailing slashes (except root "/")
    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
        path[--len] = '\0';
    }

    // Find last slash or backslash
    char *last = strrchr(path, '/');
    char *last_back = strrchr(path, '\\');

    char *cut = last;
    if (!cut || (last_back && last_back > last)) {
        cut = last_back;
    }

    if (cut) {
        // Handle root case like "/" or "C:\"
        if (cut == path || (cut == path + 2 && path[1] == ':')) {
            cut[1] = '\0';
        } else {
            *cut = '\0';
        }
    } else {
        // No separator found → current directory becomes "."
        strcpy(path, ".");
    }
}

void FileExplorer_up_one_level(ExplorerWindow * self){
  char * s = make_string(self->path);
  get_parent(s);
  FileExplorer_list_files(self, s);
  free(s);
}

void FileExplorer_send_sequence(struct Window *win, const char *seq, int len){
  LOG_INFO("FileExplorer_send_sequence: %s", seq);
}

void FileExplorer_scroll_wheel_down(struct Window *w){
  ExplorerWindow * self = w;
  Slider_scroll_down(self->slider);
}

void FileExplorer_scroll_wheel_up(struct Window *w){
  ExplorerWindow * self = w;
  Slider_scroll_up(self->slider);
}

void FileExplorer_send_key(Window * win, char c)
{
    ExplorerWindow * self = win;
    if (c == 106){ // j
        FileExplorer_select_item(self, self->selected->next);
        return;
    }
    if (c == 107){ // k
        FileExplorer_select_item(self, self->selected->prev);
        return;
    }
    if (c == 117){ // u
        Window * selected = self->selected;
        for (int i=0;i<win->calculated.height && selected->next;i++) selected = selected->next;
        FileExplorer_select_item(self, selected);
        return;
    }
    if (c == 105){ // i
        Window * selected = self->selected;
        for (int i=0;i<win->calculated.height && selected->prev;i++) selected = selected->prev;
        FileExplorer_select_item(self, selected);
        return;
    }
    if (c == 109){ // m
        Window * selected = self->selected;
        item_clicked(selected, 0, 0);
        return;
    }
    if (c == 111){ // o
        Window * selected = self->selected;
        //item_clicked(selected, 0, 0);
        char * file_path = selected->data2;
        Editor_last_open_file(file_path);
        return;
    }
    if (c == 47){ // /
        FileExplorer_up_one_level(self);
        return;
    }

    Window *focused_cursor = win->focused;
    if (focused_cursor != NULL) while (focused_cursor->send_key == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;

    if (focused_cursor != NULL && focused_cursor->send_key != NULL) {
        focused_cursor->send_key(focused_cursor, c);
    }

}

void Explorer_change_color_hover(Window *wg, int x, int y)
{
    wg->fg = 232;
    wg->bg = 27;
}

void Explorer_change_color_normal(Window *wg, int x, int y)
{
    wg->fg = 232;
    wg->bg = 255;
}

void FileExplorer_shortcut_set_target(ExplorerWindow * self, Window * shortcut, char * path){
  shortcut->on_hover = Explorer_change_color_hover;
  shortcut->undo_on_hover = Explorer_change_color_normal;
  shortcut->lambda = create_lambda(FileExplorer_list_files, 2, self, path);
  shortcut->on_mouse_down = Window_execute_lambda;
}

ExplorerWindow *FileExplorer_file_list(){
  ExplorerWindow *w = malloc(sizeof *w);
  Window_init(w, -1, -1, -1, -1, -1, -1);
  w->win.id = malloc(ID_LENGTH*4);
  snprintf(w->win.id, ID_LENGTH*4, "file list");
  //strcpy(w->win.id, "file.txt");
  //w->win.id = "file list";
  int j = 0;

  w->sort_by = -1;

  Window_add_widget(w, 0, 0, j, -1, -1, 1, w->path_label, 232, 255);
  j++;

  // favorites
  int start_j = 0;
  int fav_width = 22;
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Favorites", 255, 245);
  Window * shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🏠 Home", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/Users/jordicolomer");

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📥 Downloads", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/Users/jordicolomer/Downloads");

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📄 Documents", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/Users/jordicolomer/Documents");

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📷 Pictures", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/Users/jordicolomer/Pictures");

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎵 Music", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/Users/jordicolomer/Music");

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎬 Movies", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/Users/jordicolomer/Movies");

  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, "", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Locations", 255, 245);

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 💻 Root", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/");

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 👥 Users", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, "/Users");

  while (j <= 200)
    Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, "", 232, 255);

  // headers
  int bg = 244;
  Window * name_col = Window_add_widget(w, fav_width, -33, 1, -1, -1, 1, "   Name", 255, bg);
  //name->lambda = create_lambda(sort_list, 1, fm);
  name_col->lambda = create_lambda(FileExplorer_sort, 2, w, SORT_BY_PATH);
  name_col->on_mouse_down = Window_execute_lambda;
  Window * date_col = Window_add_widget(w, -34,       -12, 1, -1, -1, 1, "┃Date Modified", 255, bg);
  date_col->lambda = create_lambda(FileExplorer_sort, 2, w, SORT_BY_DATE);
  date_col->on_mouse_down = Window_execute_lambda;
  Window * size_col = Window_add_widget(w, -13,        0,  1, -1, -1, 1, "┃Size", 255, bg);
  size_col->lambda = create_lambda(FileExplorer_sort, 2, w, SORT_BY_SIZE);
  size_col->on_mouse_down = Window_execute_lambda;

  // markers
  Window * name_sort_marker = Window_add_widget(w, -35, -1, 1, -1, 1, 1, "▲", 255, bg);
  name_sort_marker->hidden = 1;
  w->sort_marker[0] = name_sort_marker;

  Window * name_sort_marker_inv = Window_add_widget(w, -35, -1, 1, -1, 1, 1, "▼", 255, bg);
  name_sort_marker_inv->hidden = 1;
  w->sort_marker[1] = name_sort_marker_inv;

  Window * date_sort_marker = Window_add_widget(w, -14, -1, 1, -1, 1, 1, "▲", 255, bg);
  date_sort_marker->hidden = 1;
  w->sort_marker[2] = date_sort_marker;

  Window * date_sort_marker_inv = Window_add_widget(w, -14, -1, 1, -1, 1, 1, "▼", 255, bg);
  date_sort_marker_inv->hidden = 1;
  w->sort_marker[3] = date_sort_marker_inv;

  // todo: this -2 should me -1
  Window * size_sort_marker = Window_add_widget(w, -2, -1, 1, -1, 1, 1, "▲", 255, bg);
  size_sort_marker->hidden = 1;
  w->sort_marker[4] = size_sort_marker;

  Window * size_sort_marker_inv = Window_add_widget(w, -2, -1, 1, -1, 1, 1, "▼", 255, bg);
  size_sort_marker_inv->hidden = 1;
  w->sort_marker[5] = size_sort_marker_inv;
      
  Window *fm = malloc(sizeof *fm);
  w->win.data = fm;
  w->fm = fm;
  Window_init(fm, 0, 0, 0, 0, -1, -1);

  FileExplorer_list_files(w, "/Users/jordicolomer");

  Window * fm_slider = slider_new(fm);
  fm_slider->left = fav_width;
  fm_slider->right = 0;
  fm_slider->top = 2;
  fm_slider->bottom = 0;
  Window_append(w, fm_slider);
  w->win.data2 = fm_slider;
  w->slider = fm_slider;

  w->win.send_key = FileExplorer_send_key;
  w->win.send_sequence = FileExplorer_send_sequence;
  w->win.scroll_wheel_up = FileExplorer_scroll_wheel_up;
  w->win.scroll_wheel_down = FileExplorer_scroll_wheel_down;

  w->win.bg = 15;
  w->win.fill = 1;

  return w;
}


Window *FileExplorer_menu_new(ExplorerFrame *self)
{
    LOG_INFO("FileExplorer_menu_new %p", self);
}

void FileExplorer_sort_by(ExplorerFrame *self, int sort_by){
  FileExplorer_sort(self->tabs->focused, sort_by);
}

void ExplorerFrame_up_one_level(ExplorerFrame *self){
  FileExplorer_up_one_level(self->tabs->focused);
}

Window *FileExplorer_menu(ExplorerFrame *self)
{
    Window *menu = Menu_create_horizontal();

    Window *file = Menu_create_vertical(self);
    Menu_add_element(file, "    New Window Ctrl+N", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(file, "", NULL);
    Menu_add_element(file, "    New Tab    Ctrl+N", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(file, " ❌ Close Tab  Ctrl+W", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(file, "", NULL);
    Menu_add_element(file, " 📄 New File   Ctrl+N", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(file, " 📁 New Folder Ctrl+N", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(file, "", NULL);
    Menu_add_submenu(menu, " File ", file);

    Window *edit = Menu_create_vertical(self);
    Menu_add_element(edit, " 🔪 Cut            Ctrl+X", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, " 📋 Copy           Ctrl+C", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, " 📌 Paste          Ctrl+V", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, " ❌ Delete         Backspace", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, " 📝 Rename         Ctrl+R", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, "", NULL);
    Menu_add_element(edit, " 📋 Copy Name      Ctrl+C", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, " 📋 Copy Directory Ctrl+C", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, " 📋 Copy Path      Ctrl+C", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(edit, "", NULL);
    Menu_add_submenu(menu, " Edit ", edit);

    Window *view = Menu_create_vertical(self);
    //Menu_add_element(view, " ⤶ Word wrap", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(view, " ↓ Sort By Name",          create_lambda(FileExplorer_sort_by, 2, self, SORT_BY_PATH));
    Menu_add_element(view, " ↓ Sort By Date Modified", create_lambda(FileExplorer_sort_by, 2, self, SORT_BY_DATE));
    Menu_add_element(view, " ↓ Sort By Size",          create_lambda(FileExplorer_sort_by, 2, self, SORT_BY_SIZE));
    Menu_add_element(view, "", NULL);
    Menu_add_submenu(menu, " View ", view);

    Menu_add_windows(menu, " Tabs ", self->tabs->data, self);

    return menu;
}

Window *FileExplorer_toolbar(ExplorerFrame *self)
{
    Window *toolbar = Menu_create_horizontal();
    //Menu_add_element(toolbar, " 📄 New ", create_lambda(FileExplorer_menu_new, 1, self));
    //Menu_add_element(toolbar, " ❌ Close ", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(toolbar, " ❌ Delete ", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(toolbar, " 🔪 Cut ", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(toolbar, " 📋 Copy ", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(toolbar, " 📌 Paste ", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(toolbar, " 📝 Rename ", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(toolbar, " 🔼 Up ", create_lambda(ExplorerFrame_up_one_level, 1, self));
    Menu_add_element(toolbar, " 📝 Edit ", create_lambda(ExplorerFrame_up_one_level, 1, self));
    Menu_add_element(toolbar, " 💻 Terminal ", create_lambda(ExplorerFrame_up_one_level, 1, self));

    toolbar->top = 1;

    return toolbar;
}

Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height)
{
  ExplorerFrame *frame = malloc(sizeof *frame);
  Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 0);

  /*Window * menu = FileExplorer_menu();
  Window_append(w, menu);
  Window * toolbar = FileExplorer_toolbar();
  Window_append(w, toolbar);*/

  Window *tabs = Tab_new((Window *(*)(void))FileExplorer_file_list, 1);
  tabs->top = 2;
  tabs->bottom = 0;
  tabs->left = 0;
  tabs->right = 0;
  Window_append(w, tabs);
  frame->win.focused = tabs;
  frame->tabs = tabs;

  Window *toolbar = FileExplorer_toolbar(frame);
  Window_append(w, toolbar);
  Window *menu = FileExplorer_menu(frame);
  Window_append(w, menu);

  return frame;
}
