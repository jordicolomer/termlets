#ifdef _WIN32
    /* Disable strict pointer type warnings on Windows for this file */
    #pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    #define NOMINMAX  /* Prevent Windows from defining min/max macros */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#ifdef _WIN32
    #include <windows.h>
    #include <sys/stat.h>
    #include <stdarg.h>

    /* Provide asprintf for Windows */
    static int asprintf(char **strp, const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int len = _vscprintf(fmt, args);
        if (len < 0) {
            va_end(args);
            return -1;
        }
        *strp = malloc(len + 1);
        if (!*strp) {
            va_end(args);
            return -1;
        }
        int result = vsprintf(*strp, fmt, args);
        va_end(args);
        return result;
    }

    /* Define DT_DIR for Windows */
    #define DT_DIR 4

    /* Minimal dirent.h replacement for Windows */
    struct dirent {
        char d_name[MAX_PATH];
        unsigned char d_type;
    };

    typedef struct {
        HANDLE handle;
        WIN32_FIND_DATAA find_data;
        struct dirent entry;
        int first;
    } DIR;

    static DIR *opendir(const char *name) {
        DIR *dir = malloc(sizeof(DIR));
        if (!dir) return NULL;

        char search_path[MAX_PATH];
        snprintf(search_path, sizeof(search_path), "%s\\*", name);

        dir->handle = FindFirstFileA(search_path, &dir->find_data);
        if (dir->handle == INVALID_HANDLE_VALUE) {
            free(dir);
            return NULL;
        }
        dir->first = 1;
        return dir;
    }

    static struct dirent *readdir(DIR *dir) {
        if (!dir) return NULL;

        if (dir->first) {
            dir->first = 0;
        } else {
            if (!FindNextFileA(dir->handle, &dir->find_data)) {
                return NULL;
            }
        }

        strncpy(dir->entry.d_name, dir->find_data.cFileName, MAX_PATH);
        dir->entry.d_type = (dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : 0;
        return &dir->entry;
    }

    static int closedir(DIR *dir) {
        if (!dir) return -1;
        FindClose(dir->handle);
        free(dir);
        return 0;
    }
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

#include "window.h"
#include "frame.h"
#include "slider.h"
#include "file_manager.h"
#include "tabs.h"
#include "logger.h"
#include "editor.h"
#include "buffer.h"
#include "menu.h"
#include "taskbar.h"
#include "clipboard.h"
#include "dialog.h"
#include "file_operations.h"
#include "text_edit.h"
//#include "mystring.h"
#include "common.h"
#include "config.h"

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


void FileExplorer_bg_set_item(Window * item, int bg){
  if (item == NULL) return;
  Window * current = item->head;
  while(current != NULL){
    current->bg = bg;
    current = current->next;
  }
}

void FileExplorer_paint_selection_item(FileItemWindow * item){
  if (item == NULL) return;
  if (item->is_selected == 0) FileExplorer_bg_set_item((Window *)item, 255);
  if (item->is_selected == 1) FileExplorer_bg_set_item((Window *)item, 27);
}

void FileExplorer_unselect_item(FileItemWindow * item){
  if (item == NULL) return;
  if (item->is_selected == 0) return;
  item->is_selected = 0;
  FileExplorer_paint_selection_item(item);
}

void FileExplorer_select_item(FileItemWindow * item){
  if (item == NULL) return;
  if (item->is_selected == 1) return;
  item->is_selected = 1;
  FileExplorer_paint_selection_item(item);
}

void FileExplorer_toggle_selection_item(ExplorerWindow * self, FileItemWindow * item){
  if (item == NULL) return;
  item->is_selected = 1-item->is_selected;
  if (item->is_selected == 1) self->selected = item;
  FileExplorer_paint_selection_item(item);
}

void FileExplorer_unselect_all(ExplorerWindow * self){
  if (self->fm == NULL) return;
  self->selected = NULL;
  FileItemWindow * item = (FileItemWindow *)self->fm->head;
  while (item != NULL){
    FileExplorer_unselect_item(item);
    item = (FileItemWindow *)item->win.next;
  }
}

void FileExplorer_select_single_item(ExplorerWindow * self, FileItemWindow * item){
  LOG_INFO("FileExplorer_select_single_item %p %p", self, item);
  if (item == NULL) return;
  //FileExplorer_unselect_item(self->selected);
  FileExplorer_unselect_all(self);

  self->selected = item;
  //item->bg = 27;
  FileExplorer_select_item(item);
  Slider_make_visible(self->slider, &item->win);
  Slider_show_grip(self->slider);
}

void item_clicked(Window *wg, int x, int y)
{
  LOG_INFO("item_clicked %p %d %d", wg, x, y);
    if (wg == NULL) {
      LOG_INFO("item_clicked: wg is NULL");
      return;
    }
    ExplorerWindow * self = wg->data;
    if (self == NULL) {
      LOG_INFO("item_clicked: self is NULL");
      return;
    }
    FileItemWindow *file_item = wg->data2;
    if (file_item == NULL) {
      LOG_INFO("item_clicked: file_item is NULL");
      return;
    }
    LOG_INFO("item_clicked: self=%p file_item=%p is_dir=%d path=%s", self, file_item, file_item->is_dir, file_item->path);
    if (self->selected == file_item){
      LOG_INFO("item_clicked: double-click detected");
      if (file_item->is_dir){
        LOG_INFO("item_clicked: opening directory %s", file_item->path);
        FileExplorer_list_files(self, file_item->path);
      } else {
        LOG_INFO("item_clicked: opening file %s", file_item->path);
        Editor_last_open_file(file_item->path);
      }
    } else {
      LOG_INFO("item_clicked: selecting item");
      FileExplorer_select_single_item(self, file_item);
    }

}

void command_item_clicked(Window *wg, int x, int y)
{
  LOG_INFO("command_item_clicked %p %d %d", wg, x, y);
    ExplorerWindow * self = wg->data;
    FileItemWindow *file_item = wg->data2;
    
    FileExplorer_toggle_selection_item(self, file_item);
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

void FileExplorer_refresh(ExplorerWindow * self){
  LOG_INFO("FileExplorer_refresh %p %s", self, self->path);
  FileExplorer_list_files(self, self->path);
}

void FileExplorer_list_files(ExplorerWindow * self, char * dire){
  snprintf(self->path_label, 1024, " 📁 %s", dire);
  Window_set_id_from_path(self, "📁", dire);

  if (self->path != dire){
    //if (self->path != NULL) free(self->path);
    self->path = make_string(dire);
  }

  LOG_INFO("FileExplorer_list_files: %p %s filename:%s", self, dire, self->win.id);
  self->selected = NULL;
  
  Window *fm = self->fm;
  Window *slider = self->slider;
  if (slider != NULL)
  {
    Slider_reset(slider);
  }
  fm->virtual_height = 0;
  fm->shift = 0;

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
    // Avoid double slash when dire is root "/"
    int len;
    if (strcmp(dire, "/") == 0)
      len = asprintf(&full_path, "/%s", entry->d_name);
    else
      len = asprintf(&full_path, "%s/%s", dire, entry->d_name);

	LOG_INFO("readdir: %s", entry->d_name);
    FileItemWindow *file_item = malloc(sizeof *file_item);
    memset(file_item, 0, sizeof *file_item);
    file_item->is_dir = 0;
    char *icon = "📄";
    if (entry->d_type == DT_DIR)
    {
      icon = "📁";
      file_item->is_dir = 1;
    }
    if (ends_with(entry->d_name, ".png"))
    {
      //icon = "🏞";
      icon = "📷";
    }
    if (ends_with(entry->d_name, ".pdf"))
    {
      icon = "📖";
    }
    char *str = NULL;
    // memory leak here

    char * date = malloc(32);
    char * size = malloc(16);
    // Initialize with empty values in case stat fails
    strcpy(date, "");
    strcpy(size, "");
    /*char date[32];
    date[0] = 0;
    char size[16];
    size[0] = 0;*/
    struct stat st;
    file_item->path = full_path;
    file_item->name = make_string(entry->d_name);
    if (stat(full_path, &st) == 0){
      struct tm *tm = localtime(&st.st_mtime);
      strftime(date, 32, "%Y-%m-%d %H:%M:%S", tm);

      human_size(st.st_size, size, 16);

      file_item->size = st.st_size;
      file_item->date = st.st_mtime;

    } else {
      LOG_INFO("stat failed for %s: %s", full_path, strerror(errno));
    }
    remove_newlines(entry->d_name);


    int filename_width = 20;
    //if (fm->calculated.width != 0) filename_width = fm->calculated.width - 48;
    //LOG_INFO("fm->calculated.width: %d", fm->calculated.width);
    //len = asprintf(&str, "%s %*s %s  %10s", icon, -filename_width, entry->d_name, date, size);
    len = asprintf(&str, "%s %s", icon, entry->d_name);
    // Window_add_widget(w, fav_width, 0, j++, -1, -1, 1, str, 232, 255);

    Window_init(&file_item->win, -1, -1, -1, -1, -1, -1);
    file_item->win.left = 0;
    file_item->win.right = 0;
    file_item->win.top = j;
    file_item->win.height = 1;
    Window_append(fm, &file_item->win);

    Window * item = Window_add_widget(&file_item->win, 0, -20, 0, -1, -1, 1, str, 232, 255);

    Window_add_widget(&file_item->win, -33, 0, 0, -1, -1, 1, date, 232, 255);
    Window_add_widget(&file_item->win, -12, 0, 0, -1, -1, 1, size, 232, 255);
    /*Window * item = Window_add_widget(file_item, 0, -20, 0, -1, -1, 1, str, 232, 255);
    Window_add_widget(file_item, -20, -10, 0, -1, -1, 1, date, 232, 255);
    Window_add_widget(file_item, -10, 0, 0, -1, -1, 1, size, 232, 255);*/
    //Window * date_item = Window_add_widget(fm, 0, 32, j, -1, -1, 1, date, 232, 255);
    j++;



    item->data = self;
    //item->data2 = full_path;
    item->data2 = file_item;
    item->on_mouse_down = item_clicked;
    item->on_command_mouse_down = command_item_clicked;

    file_item->win.data = self;
    file_item->win.data2 = full_path;

    if (self->selected == NULL) FileExplorer_select_single_item(self, file_item);
    //LOG_INFO("full_path: %s", full_path);
    // if (height < j) break;
    // mvwprintw(win, x++, 1, "%s %s", icon, entry->d_name);
  }
  closedir(dir);

  // Update virtual_height to reflect the actual number of files
  fm->virtual_height = j;
  LOG_INFO("FileExplorer_list_files: added %d files, virtual_height=%d, calculated.height=%d, fm->head=%p", j, fm->virtual_height, fm->calculated.height, fm->head);

  //sort_list(fm);
  int sort_by = self->sort_by;
  self->sort_by = -1;
  FileExplorer_sort(self, sort_by);
  FileExplorer_select_single_item(self, (FileItemWindow *)fm->head);

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

void FileExplorer_new_file(ExplorerWindow * self){
  // find free filename
  char path[PATH_MAX];
  int n = 1;
  while (1)
  {
      if (n == 1)
      {
          snprintf(path, sizeof(path), "%s/new file.txt", self->path);
      }
      else
      {
          snprintf(path, sizeof(path), "%s/new file %d.txt", self->path, n);
      }

      if (!file_exists(path))
      {
          //printf("Available filename: %s\n", path);
          break;
      }
      n++;
  }


  // create file
  FILE *file = fopen(path, "w");
  if (file == NULL) {
      perror("fopen");
      return;
  }
  fclose(file);

  FileExplorer_refresh(self);
}

void FileExplorer_copy_name(ExplorerWindow * self){
  if (self->selected != NULL)
    clipboard_copy(self->selected->name);
}

void FileExplorer_copy_directory(ExplorerWindow * self){
  clipboard_copy(self->path);
}

void FileExplorer_copy_path(ExplorerWindow * self){
  if (self->selected != NULL)
    clipboard_copy(self->selected->path);
}

void FileExplorer_edit(ExplorerWindow * self){
  if (self->selected != NULL)
    Editor_last_open_file(self->selected->path);
}

void FileExplorer_terminal(ExplorerWindow * self){
  //Editor_last_open_file(self->selected->path);
  vterminal_new();
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

Window *ExplorerWindow_rename_complete(ExplorerWindow *self, LineEditorWindow * line_edit, char * origin)
{
  //rename();
  char * target = NULL;
  
  char * parent = make_string(origin);
  get_parent(parent);
  asprintf(&target, "%s/%s", parent, line_edit->buffer);
  free(parent);
  //LOG_INFO("ExplorerWindow_rename_complete: %s %s", origin, target);
  rename(origin, target);
  free(target);

  Window_remove(line_edit);
  self->win.focused = NULL;
  FileExplorer_refresh(self);
}

Window *ExplorerWindow_rename(ExplorerWindow *self)
{
  FileItemWindow * selected = self->selected;
  if (selected == NULL) return NULL;

  // create line edit and copy geometry from label
  LineEditorWindow * line_edit = LineEditorWindow_new(selected->name);
  Window * label = selected->win.head;
  line_edit->win.left = selected->win.left+3; // don't include icon
  line_edit->win.right = selected->win.right;
  line_edit->win.top = selected->win.top;
  line_edit->win.bottom = selected->win.bottom;
  line_edit->win.width = selected->win.width;
  line_edit->win.height = selected->win.height;
  line_edit->win.lambda = create_lambda(ExplorerWindow_rename_complete, 3, self, line_edit, selected->path);

  self->win.focused = line_edit;

  Window_append(self->fm, line_edit);
}

void FileExplorer_send_key(Window * win, char c)
{
    ExplorerWindow * self = win;

    Window *focused_cursor = win->focused;
    if (focused_cursor != NULL) while (focused_cursor->send_key == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;

    if (focused_cursor != NULL && focused_cursor->send_key != NULL) {
        focused_cursor->send_key(focused_cursor, c);
        return;
    }
    Action action = mapping[c];

    //if (action == ACTION_DOWN){
    if (c == 106){ // j
        if (self->selected != NULL && self->selected->win.next != NULL)
            FileExplorer_select_single_item(self, (FileItemWindow *)self->selected->win.next);
        return;
    }
    if (c == 107){ // k
	  //if (action == ACTION_UP){
        if (self->selected != NULL && self->selected->win.prev != NULL)
            FileExplorer_select_single_item(self, (FileItemWindow *)self->selected->win.prev);
        return;
    }
    //if (action == ACTION_PAGE_UP){
    if (c == 117){ // u
        if (self->selected != NULL) {
            Window * selected = &self->selected->win;
            for (int i=0;i<win->calculated.height && selected->next;i++) selected = selected->next;
            FileExplorer_select_single_item(self, (FileItemWindow *)selected);
        }
        return;
    }
    //if (action == ACTION_PAGE_DOWN){
    if (c == 105){ // i
        if (self->selected != NULL) {
            Window * selected = &self->selected->win;
            for (int i=0;i<win->calculated.height && selected->prev;i++) selected = selected->prev;
            FileExplorer_select_single_item(self, (FileItemWindow *)selected);
        }
        return;
    }
    //if (action == ACTION_ENTER){
    if (c == 13){ // CR
        FileItemWindow * selected = self->selected;
        if (selected != NULL && selected->win.head != NULL) {
            item_clicked(selected->win.head, 0, 0);
        }
        return;
    }
    if (c == 'e'){
        FileItemWindow * selected = self->selected;
        //item_clicked(selected, 0, 0);
        if (selected != NULL) {
            char * file_path = selected->path;
            Editor_last_open_file(file_path);
        }
        return;
    }
    if (c == 47){ // /
        FileExplorer_up_one_level(self);
        return;
    }
    if (c == 'r'){ // r
        ExplorerWindow_rename(self);
        return;
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
  memset(w, 0, sizeof *w);  // Zero-initialize to prevent garbage values
  Window_init(w, -1, -1, -1, -1, -1, -1);
  w->win.id = malloc(ID_LENGTH*4);
  snprintf(w->win.id, ID_LENGTH*4, "file list");
  //strcpy(w->win.id, "file.txt");
  //w->win.id = "file list";
  int j = 0;

  w->sort_by = -1;

  Window_add_widget(w, 0, 0, j, -1, -1, 1, w->path_label, 232, 255);
  j++;

  // Get home directory
#ifdef _WIN32
  const char *home_dir = getenv("USERPROFILE");
  if (home_dir == NULL) home_dir = "C:/";
#else
  const char *home_dir = getenv("HOME");
  if (home_dir == NULL) home_dir = "/";
#endif

  // Build paths for shortcuts
  char *home_path = malloc(PATH_MAX);
  char *downloads_path = malloc(PATH_MAX);
  char *documents_path = malloc(PATH_MAX);
  char *pictures_path = malloc(PATH_MAX);
  char *music_path = malloc(PATH_MAX);
  char *movies_path = malloc(PATH_MAX);
  char *dropbox_path = malloc(PATH_MAX);

  snprintf(home_path, PATH_MAX, "%s", home_dir);
  snprintf(downloads_path, PATH_MAX, "%s/Downloads", home_dir);
  snprintf(documents_path, PATH_MAX, "%s/Documents", home_dir);
  snprintf(pictures_path, PATH_MAX, "%s/Pictures", home_dir);
  snprintf(music_path, PATH_MAX, "%s/Music", home_dir);
  snprintf(movies_path, PATH_MAX, "%s/Movies", home_dir);
  snprintf(dropbox_path, PATH_MAX, "%s/Dropbox", home_dir);

  // favorites
  int start_j = 0;
  int fav_width = 22;
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Favorites", 255, 245);

  struct stat st;
  Window * shortcut;

  // Home - always create
  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🏠 Home", 232, 254);
  FileExplorer_shortcut_set_target(w, shortcut, home_path);

  // Downloads
  if (stat(downloads_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📥 Downloads", 232, 254);
    FileExplorer_shortcut_set_target(w, shortcut, downloads_path);
  } else {
    free(downloads_path);
  }

  // Documents
  if (stat(documents_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📄 Documents", 232, 254);
    FileExplorer_shortcut_set_target(w, shortcut, documents_path);
  } else {
    free(documents_path);
  }

  // Pictures
  if (stat(pictures_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📷 Pictures", 232, 254);
    FileExplorer_shortcut_set_target(w, shortcut, pictures_path);
  } else {
    free(pictures_path);
  }

  // Music
  if (stat(music_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎵 Music", 232, 254);
    FileExplorer_shortcut_set_target(w, shortcut, music_path);
  } else {
    free(music_path);
  }

  // Movies
  if (stat(movies_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎬 Movies", 232, 254);
    FileExplorer_shortcut_set_target(w, shortcut, movies_path);
  } else {
    free(movies_path);
  }

  // Dropbox
  if (stat(dropbox_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📦 Dropbox", 232, 254);
    FileExplorer_shortcut_set_target(w, shortcut, dropbox_path);
  } else {
    free(dropbox_path);
  }

  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, "", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Locations", 255, 245);

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 💻 Root", 232, 254);
#ifdef _WIN32
  FileExplorer_shortcut_set_target(w, shortcut, "C:/");
#else
  FileExplorer_shortcut_set_target(w, shortcut, "/");
#endif

  shortcut = Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 👥 Users", 232, 254);
#ifdef _WIN32
  FileExplorer_shortcut_set_target(w, shortcut, "C:/Users");
#else
  FileExplorer_shortcut_set_target(w, shortcut, "/Users");
#endif

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

  FileExplorer_list_files(w, home_path);

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

  w->win.bg = 255;
  w->win.fill = 1;

  return w;
}

Window *ExplorerFrame_refresh(ExplorerFrame *self)
{
  ExplorerWindow * ew = self->tabs->focused;
  FileExplorer_refresh(ew);
}

Window *FileExplorer_menu_delete_execute(ExplorerFrame *self, ExplorerWindow * ew)
{
  LOG_INFO("FileExplorer_menu_delete_execute %p %p", self, ew);
  
  FileItemWindow * current = (FileItemWindow *)ew->fm->head;
  while (current != NULL){
    if (current->is_selected){
      remove(current->path);
    }
    current = (FileItemWindow *)current->win.next;
  }
  ExplorerFrame_refresh(self);
}

Window *FileExplorer_menu_delete(ExplorerFrame *self)
{
  ExplorerWindow * ew = self->tabs->focused;
  if (ew->selected == NULL) return NULL;
  char * path = ew->selected->path;
  LOG_INFO("FileExplorer_menu_delete %p", self);
  open_dialog(
    self,
    "Are you sure?",
    create_lambda(FileExplorer_menu_delete_execute, 2, self, ew));
}

//char * paste_path = NULL;
//char * paste_name = NULL;
ExplorerWindow * paste_source = NULL;
int paste_operation = 0;

/*void string_set(char * target, char * origin){
  if (target == origin) return;

  if (target != NULL) free(target);
  target = make_string(origin);
}*/
void string_set(char **target, const char *origin)
{
    if (*target == origin)
        return;

    free(*target);          // free(NULL) is safe
    *target = make_string(origin);
}

Window *FileExplorer_menu_cut(ExplorerFrame *self)
{
  ExplorerWindow * ew = self->tabs->focused;
  paste_source = ew;
  //string_set(&paste_path, ew->selected->path);
  //string_set(&paste_name, ew->selected->name);
  paste_operation = 1;
}

Window *FileExplorer_menu_copy(ExplorerFrame *self)
{
  ExplorerWindow * ew = self->tabs->focused;
  paste_source = ew;
  //string_set(&paste_path, ew->selected->path);
  //string_set(&paste_name, ew->selected->name);
  paste_operation = 2;
}

Window *FileExplorer_menu_paste(ExplorerFrame *self)
{
  if (paste_source == NULL) return NULL;
  if (paste_source->fm == NULL) return NULL;
  ExplorerWindow * ew = self->tabs->focused;
  char * destination = ew->path;
  FileItemWindow * current = (FileItemWindow *)paste_source->fm->head;

  while (current != NULL){
    if (current->is_selected == 1){
      char * dst = NULL;
      asprintf(&dst, "%s/%s", destination, current->name);
      LOG_INFO("FileExplorer_menu_paste cut %s %s", current->path, dst);
      if (paste_operation == 1) rename(current->path, dst);
      if (paste_operation == 2) copy_file(current->path, dst);
      free(dst);
    }
    current = (FileItemWindow *)current->win.next;
  }
  
  FileExplorer_refresh(ew);
  FileExplorer_refresh(paste_source);
}

Window *FileExplorer_menu_new(ExplorerFrame *self)
{
    LOG_INFO("FileExplorer_menu_new %p", self);
}

void FileExplorer_sort_by(ExplorerFrame *self, int sort_by){
  FileExplorer_sort(self->tabs->focused, sort_by);
}

/*void ExplorerFrame_up_one_level(ExplorerFrame *self){
  FileExplorer_up_one_level(self->tabs->focused);
}*/

void ExplorerFrame_on_selected(ExplorerFrame *self, void fn()){
  //FileExplorer_up_one_level(self->tabs->focused);
  fn(self->tabs->focused);
}



Window *FileExplorer_menu_rename(ExplorerFrame *self)
{
  LOG_INFO("FileExplorer_menu_rename %p", self);
  ExplorerWindow * ew = self->tabs->focused;
  ExplorerWindow_rename(ew);
}

Window *FileExplorer_menu(ExplorerFrame *self)
{
    Window *menu = Menu_create_horizontal();

    Window *file = Menu_create_vertical(self);
    Menu_add_element(file, " 📄 New File   Ctrl+N", create_lambda(ExplorerFrame_on_selected, 2, self, FileExplorer_new_file));
    Menu_add_element(file, " 📁 New Folder Ctrl+N", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(file, "    New Window Ctrl+N", create_lambda(file_manager_new, 0));
    Menu_add_element(file, "    New Tab    Ctrl+N", create_lambda(tabs_new_tab, 1, self->tabs));
    Menu_add_element(file, "", NULL);
    Menu_add_element(file, " ❌ Close Window Ctrl+N", create_lambda(Frame_close, 1, self));
    Menu_add_element(file, " ❌ Close Tab  Ctrl+W", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(file, "", NULL);
    Menu_add_submenu(menu, " File ", file);

    Window *edit = Menu_create_vertical(self);
    Menu_add_element(edit, " 🔪 Cut            Ctrl+X", create_lambda(FileExplorer_menu_cut, 1, self));
    Menu_add_element(edit, " 📋 Copy           Ctrl+C", create_lambda(FileExplorer_menu_copy, 1, self));
    Menu_add_element(edit, " 📌 Paste          Ctrl+V", create_lambda(FileExplorer_menu_paste, 1, self));
    Menu_add_element(edit, " ❌ Delete         Backspace", create_lambda(FileExplorer_menu_delete, 1, self));
    Menu_add_element(edit, " 📝 Rename         Ctrl+R", create_lambda(FileExplorer_menu_rename, 1, self));
    Menu_add_element(edit, "", NULL);
    Menu_add_element(edit, " 📋 Copy Name      Ctrl+C", create_lambda(ExplorerFrame_on_selected, 2, self, FileExplorer_copy_name));
    Menu_add_element(edit, " 📋 Copy Directory Ctrl+C", create_lambda(ExplorerFrame_on_selected, 2, self, FileExplorer_copy_directory));
    Menu_add_element(edit, " 📋 Copy Path      Ctrl+C", create_lambda(ExplorerFrame_on_selected, 2, self, FileExplorer_copy_path));
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
    Menu *toolbar_menu = (Menu *)Menu_create_horizontal();
    //Menu_add_element(toolbar_menu, " 📄 New ", create_lambda(FileExplorer_menu_new, 1, self));
    //Menu_add_element(toolbar_menu, " ❌ Close ", create_lambda(FileExplorer_menu_new, 1, self));
    Menu_add_element(toolbar_menu, " ❌ Delete ", create_lambda(FileExplorer_menu_delete, 1, self));
    Menu_add_element(toolbar_menu, " 🔪 Cut ", create_lambda(FileExplorer_menu_cut, 1, self));
    Menu_add_element(toolbar_menu, " 📋 Copy ", create_lambda(FileExplorer_menu_copy, 1, self));
    Menu_add_element(toolbar_menu, " 📌 Paste ", create_lambda(FileExplorer_menu_paste, 1, self));
    Menu_add_element(toolbar_menu, " 🔤 Rename ", create_lambda(FileExplorer_menu_rename, 1, self));
    Menu_add_element(toolbar_menu, " 🔄 Refresh ", create_lambda(ExplorerFrame_refresh, 1, self));
    //Menu_add_element(toolbar_menu, " 🔼 Up ", create_lambda(ExplorerFrame_up_one_level, 1, self));
    Menu_add_element(toolbar_menu, " 🔝 Up ", create_lambda(ExplorerFrame_on_selected, 2, self, FileExplorer_up_one_level));
    Menu_add_element(toolbar_menu, " 📝 Edit ", create_lambda(ExplorerFrame_on_selected, 2, self, FileExplorer_edit));
    Menu_add_element(toolbar_menu, " 💻 Terminal ", create_lambda(ExplorerFrame_on_selected, 2, self, FileExplorer_terminal));

    toolbar_menu->win.top = 1;

    return (Window *)toolbar_menu;
}

Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height)
{
  ExplorerFrame *explorer_frame = malloc(sizeof *explorer_frame);
  memset(explorer_frame, 0, sizeof *explorer_frame);  // Zero-initialize to prevent garbage values
  Window *frame = (Window *)explorer_frame;
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
  explorer_frame->win.focused = tabs;
  explorer_frame->tabs = tabs;

  Window *toolbar = FileExplorer_toolbar(explorer_frame);
  Window_append(w, toolbar);
  Window *menu = FileExplorer_menu(explorer_frame);
  Window_append(w, menu);

  return frame;
}
