#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "window.h"
#include "frame.h"
#include "slider.h"
#include "file_manager.h"

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


Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height)
{
  // Window* w = malloc(sizeof *w);
  Window *frame = malloc(sizeof *frame);
  // Window_init(w, x, y, width, height);
  Window *w = Frame_init(frame, left, right, top, bottom, width, height, NULL);
  // LOG_INFO("FileExplorer_new: %d %d %d %d %d", x, y, Window_get_absolute_x(w), Window_get_absolute_y(w), w->x);

  // w->draw = FileExplorer_draw;

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
  // Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
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
  // Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // Widget* Window_add_widget(Window* w, int left, int right, int top, int bottom, int width, int height, char * c, int fg, int bg){
  //  tabs
  x_offset = 0;
  widget_width = 14;
  Window_add_widget(w, x_offset, -1, j, -1, widget_width, 1, " jordicolomer x", 232, 255);
  x_offset += widget_width;
  widget_width = 1;
  Window_add_widget(w, x_offset, 0, j, 1, -1, 1, " + ", 232, 255);
  x_offset += widget_width;
  // Window_add_widget(w, 0, j, width, 1, "", BLACK, WHITE_BG);
  j++;

  // address bar
  Window_add_widget(w, 0, 0, j, -1, -1, 1, " 📁 /Users/jordicolomer", 232, 255);
  j++;

  // favorites
  int start_j = j;
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

  // list files

  Window *fm = malloc(sizeof *fm);
  Window_init(fm, 0, 0, 0, 0, -1, -1);

  j = 0;
  DIR *dir;
  struct dirent *entry;

  dir = opendir("/Users/jordicolomer"); // current directory

  if (dir == NULL)
  {
    perror("opendir");
    return NULL;
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
    int len = asprintf(&str, "%s %s", icon, entry->d_name);
    // Window_add_widget(w, fav_width, 0, j++, -1, -1, 1, str, 232, 255);
    Window_add_widget(fm, 0, 0, j++, -1, -1, 1, str, 232, 255);
    // if (height < j) break;
    // mvwprintw(win, x++, 1, "%s %s", icon, entry->d_name);
  }

  closedir(dir);


  Window * fm_slider = slider_new(fm, height - 4);
  fm_slider->left = fav_width;
  fm_slider->right = 0;
  fm_slider->top = 4;
  fm_slider->bottom = 0;
  Window_append(w, fm_slider);

  return frame;
}
