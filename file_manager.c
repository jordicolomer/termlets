#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "window.h"
#include "frame.h"
#include "slider.h"
#include "file_manager.h"
#include "tabs.h"
#include "logger.h"

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


void item_clicked(Window *wg, int x, int y)
{
    ExplorerWindow * self = wg->data;
    char * dire = wg->data2;
    FileExplorer_list_files(self, dire);
}

void FileExplorer_select_item(ExplorerWindow * self, Window * item){
  if (item == NULL) return;
  if (self->selected != NULL) self->selected->bg = 255;
  self->selected = item;
  item->bg = 27;
  Slider_make_visible(self->slider, item);
  Slider_show_grip(self->slider);
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

void FileExplorer_list_files(ExplorerWindow * self, char * dire){
  LOG_INFO("FileExplorer_list_files: %p %s", self, dire);
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
    char *icon = "📄";
    if (entry->d_type == DT_DIR)
    {
      icon = "📁";
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
    remove_newlines(entry->d_name);
    int len = asprintf(&str, "%s %s", icon, entry->d_name);
    // Window_add_widget(w, fav_width, 0, j++, -1, -1, 1, str, 232, 255);
    Window * item = Window_add_widget(fm, 0, 0, j++, -1, -1, 1, str, 232, 255);
    item->on_mouse_down = item_clicked;
    item->data = self;

    char *full_path = NULL;
    len = asprintf(&full_path, "%s/%s", dire, entry->d_name);
    item->data2 = full_path;

    if (self->selected == NULL) FileExplorer_select_item(self, item);
    //LOG_INFO("full_path: %s", full_path);
    // if (height < j) break;
    // mvwprintw(win, x++, 1, "%s %s", icon, entry->d_name);
  }

  closedir(dir);

  // Clear remaining lines to remove previous list items
  while (j <= self->win.calculated.height)
    Window_add_widget(fm, 0, 0, j++, -1, -1, 1, "", 232, 255);
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
    }
    if (c == 109){ // m
        Window * selected = self->selected;
        item_clicked(selected, 0, 0);
    }

    Window *focused_cursor = win->focused;
    if (focused_cursor != NULL) while (focused_cursor->send_key == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;

    if (focused_cursor != NULL && focused_cursor->send_key != NULL) {
        focused_cursor->send_key(focused_cursor, c);
    }

}

ExplorerWindow *FileExplorer_file_list(){
  ExplorerWindow *w = malloc(sizeof *w);
  Window_init(w, -1, -1, -1, -1, -1, -1);
  w->win.id = "file list";
  int j = 0;

  Window_add_widget(w, 0, 0, j, -1, -1, 1, " 📁 /Users/jordicolomer", 232, 255);
  j++;

  // favorites
  int start_j = 0;
  int fav_width = 22;
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Favorites", 255, 245);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🏠 Home", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📥 Downloads", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📄 Documents", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 📷 Pictures", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎵 Music", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 🎬 Movies", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, "", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " Locations", 255, 245);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 💻 Root", 232, 254);
  Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, " 👥 Users", 232, 254);
  while (j <= 200)
    Window_add_widget(w, 0, -1, j++, -1, fav_width, 1, "", 232, 255);
    
  Window *fm = malloc(sizeof *fm);
  w->win.data = fm;
  w->fm = fm;
  Window_init(fm, 0, 0, 0, 0, -1, -1);

  FileExplorer_list_files(w, "/Users/jordicolomer");

  Window * fm_slider = slider_new(fm);
  fm_slider->left = fav_width;
  fm_slider->right = 0;
  fm_slider->top = 1;
  fm_slider->bottom = 0;
  Window_append(w, fm_slider);
  w->win.data2 = fm_slider;
  w->slider = fm_slider;

  w->win.send_key = FileExplorer_send_key;

  return w;
}


Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height)
{
  Window *frame = malloc(sizeof *frame);
  Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL, 0);
  // LOG_INFO("FileExplorer_new: %d %d %d %d %d", x, y, Window_get_absolute_x(w), Window_get_absolute_y(w), w->x);

  int j = 0;

  int widget_width;

  // menubar
  int x_offset = 0;
  widget_width = 6;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " File", 232, 253);
  x_offset += widget_width;
  widget_width = 6;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " Edit", 232, 253);
  x_offset += widget_width;
  widget_width = 6;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " View", 232, 253);
  x_offset += widget_width;
  widget_width = 6;
  Window_add_widget(w, x_offset, 0, j, -1, -1, 1, " Help", 232, 253);
  x_offset += widget_width;
  j++;

  // toolbar
  x_offset = 0;
  widget_width = 12;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " 📄 New File", 232, 254);
  x_offset += widget_width;
  widget_width = 12;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📁 New Dir", 232, 254);
  x_offset += widget_width;
  widget_width = 8;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📋 Copy", 232, 254);
  x_offset += widget_width;
  widget_width = 8;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "🔪 Cut", 232, 254);
  x_offset += widget_width;
  widget_width = 10;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "📌 Paste", 232, 254);
  x_offset += widget_width;
  widget_width = 10;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, "🔤 Rename", 232, 254);
  x_offset += widget_width;
  widget_width = 10;
  Window_add_widget(w, x_offset, 0, j, -1, -1, 1, "❌ Delete", 232, 254);
  x_offset += widget_width;
  j++;

  Window *tabs = Tab_new((Window *(*)(void))FileExplorer_file_list);
  tabs->top = j;
  tabs->bottom = 0;
  tabs->left = 0;
  tabs->right = 0;
  Window_append(w, tabs);
  frame->focused = tabs;

  return frame;
}
