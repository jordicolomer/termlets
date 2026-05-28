#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "window.h"
#include "frame.h"
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


/*typedef struct Slider_data
{
  Window *slider_grip;
  Window *child;
  int height;
  int virtual_height;
} Slider_data;*/


void Slider_hover(Window *wg, int x, int y)
{
  Window *slider_grip = ((Slider_data *)wg->data)->slider_grip;
  slider_grip->hidden = 0;
  // LOG_INFO("Slider_hover");
}

void Slider_grip_hover(Window *wg, int x, int y)
{
  wg->hidden = 0;
}

void Slider_grip_undo_hover(Window *wg, int x, int y)
{
  wg->hidden = 1;
}

void Slider_undo_hover(Window *wg, int x, int y)
{
  Window *slider_grip = ((Slider_data *)wg->data)->slider_grip;
  slider_grip->hidden = 1;
  // LOG_INFO("Slider_hover");
}

void Slider_on_mouse_down(Window *wg, int x, int y)
{
  Slider_data *slider_data = (Slider_data *)wg->data;
  Window *child = slider_data->child;
  child->shift -= slider_data->height;
}

/*void Slider_grip_draw(struct Window *w, Geometry geo, int hasFocus)
{
  Slider_data *slider_data = (Slider_data *)w->parent->data;
  Window *child = slider_data->child;
  child->shift = -w->top;
  Widget_draw(w, geo, hasFocus);
}*/

void on_mouse_down_slider_grip(Window *wg, int x, int y)
{
  /*
  Slider_data * slider_data = (Slider_data*) wg->parent->data;
  Window* child = slider_data->child;
  child->shift -= 1;
  */

  dragging = wg;
  dragging_offset_x = x - wg->parent->left;
  // dragging_offset_y = y - wg->parent->top;
  dragging_offset_y = y - wg->parent->top - wg->top;
}

void Slider_set_top(struct Window *w, int top){
  w->top = top;

  Slider_data *slider_data = (Slider_data *)w->parent->data;
  Window *child = slider_data->child;
  int height = Window_get_height(w->parent) - 1;
  //child->shift = -w->top*(slider_data->virtual_height)/slider_data->height;
  //int height = slider_data->height-2;
  //LOG_INFO("Slider_set_top %d %d %d", height, height2, w->top);
  child->shift = -w->top*(slider_data->virtual_height-height-1)/height;
}



Window *slider_new(Window *fm, int width, int height, int virtual_height){
  Window *fm_slider = malloc(sizeof *fm_slider);
  Window_init(fm_slider, width, 0, 4, 0, -1, -1);
  Window_append(fm_slider, fm);

  Window *slider = malloc(sizeof *slider);
  Window_init(slider, -1, 0, 0, 0, 2, -1);
  Window_append(fm_slider, slider);
  slider->on_hover = Slider_hover;
  slider->undo_on_hover = Slider_undo_hover;
  slider->on_mouse_down = Slider_on_mouse_down;

  Slider_data *slider_data = (Slider_data *)malloc(sizeof(Slider_data));
  slider->data = slider_data;
  slider_data->child = fm;
  slider_data->height = height;
  slider_data->virtual_height = virtual_height;

  Window *slider_grip = Window_add_widget(slider, -1, 0, 0, -1, 2, 1, "░░", 232, 255);
  slider_grip->on_mouse_down = on_mouse_down_slider_grip;
  slider_grip->hidden = 1;
  slider_grip->on_hover = Slider_grip_hover;
  slider_grip->undo_on_hover = Slider_grip_undo_hover;
  //slider_grip->draw = Slider_grip_draw;
  slider_grip->set_top = Slider_set_top;
  slider_data->slider_grip = slider_grip;

  return fm_slider;
}

Window *FileExplorer_new(int left, int right, int top, int bottom, int width, int height)
{
  // Window* w = malloc(sizeof *w);
  Window *frame = malloc(sizeof *frame);
  // Window_init(w, x, y, width, height);
  Window *w = Frame_init(frame, left, right, top, bottom, width, height);
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


  Window * fm_slider = slider_new(fm, fav_width, height - 4, j);
  Window_append(w, fm_slider);

  return frame;
}

/*
Window *FileExplorer_test(int x, int y, int width, int height)
{
  Window *frame = malloc(sizeof *frame);
  Window *w = Frame_init(frame, x, -1, y, -1, width, height);
  return frame;
}
*/
