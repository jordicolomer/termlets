#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "window.h"
#include "logger.h"
#include "buffer.h"
#include "utils.h"
#include "lambda.h"


void Window_set_top(struct Window *wg, int top){
  wg->top = top;
}

Window *root = NULL;

Window *draggingX = NULL;
Window *draggingY = NULL;
Window *resizing = NULL;
Window *focused = NULL;
Window *hovering = NULL;
Window *open_menu = NULL;
int dragging_offset_x, dragging_offset_y;

void Window_append(Window *w, Window *new_w)
{
  new_w->parent = w;
  new_w->next = NULL;
  new_w->prev = w->tail;

  if (w->tail != NULL)
  {
    w->tail->next = new_w;
  }
  else
  {
    w->head = new_w;
  }

  w->virtual_height = max(w->virtual_height, new_w->top+new_w->height);

  w->tail = new_w;
}

void Window_remove(Window *w)
{
    if (w == NULL || w->parent == NULL)
    {
        return;
    }

    Window *parent = w->parent;

    /* Link previous node to next node */
    if (w->prev != NULL)
    {
        w->prev->next = w->next;
    }
    else
    {
        /* w was the head */
        parent->head = w->next;
    }

    /* Link next node to previous node */
    if (w->next != NULL)
    {
        w->next->prev = w->prev;
    }
    else
    {
        /* w was the tail */
        parent->tail = w->prev;
    }

    /* Fully detach w */
    w->parent = NULL;
    w->next = NULL;
    w->prev = NULL;
}


void Window_fill(struct Window *w){
  Geometry geo = w->calculated;
  int fg = w->fg;
  int bg = w->bg;
  for (int i=0;i<geo.height;i++) Buffer_print(&main_buf, geo.y+i, geo.x, geo.width, "", fg, bg);
}

// void Window_draw(struct Window* w, int bias_x, int bias_y, int hasFocus){
void Window_draw(struct Window *w, int hasFocus)
{
  if (w->fill == 1) Window_fill(w);
    Geometry geo = w->calculated;
    if (w->hidden == 1){
        return;
    }

  // LOG_INFO("Window_draw %s w:%p geo.x:%d, geo.y:%d, geo.width:%d, geo.height:%d", w->id, w, geo.x, geo.y, geo.width, geo.height);
  if (focused == w)
    hasFocus = 1;

  Window *current = w->head;
  while (current != NULL)
  {

    // geo.width = w->left + w->width + w->right

    int left = current->left;
    if (left != -1) left += w->shift_x;
    int right = current->right;
    int width = current->width;

    if (left == -1)
      left = geo.width - current->right - current->width;
    if (right == -1)
      right = geo.width - current->left - current->width;
    if (width == -1)
      width = geo.width - current->left - current->right;
    if (left < -1){
      left = geo.width + current->left;
    }
    if (right < -1){
      right = geo.width + current->right;
    }

    // geo.height = w->top + w->height + w->bottom

    int top = current->top + w->shift;
    int bottom = current->bottom;
    int height = current->height;

    if (top == -1)
      top = geo.height - current->height - current->bottom;
    if (bottom == -1)
      bottom = geo.height - current->top - current->height;
    if (height == -1)
      height = geo.height - current->top - current->bottom;

    if (left + width > geo.width){
      width = geo.width - left;
    }
    Geometry rect = {geo.x + left, geo.y + top, width, height};
    current->calculated = rect;
    //LOG_INFO("Window_draw loop %s %p orig css left:%d right:%d width:%d top:%d bottom:%d height:%d", current->id, current, current->left, current->right, current->width, current->top, current->bottom, current->height);
    //LOG_INFO("Window_draw loop %s %p comp css left:%d right:%d width:%d top:%d bottom:%d height:%d", current->id, current, left, right, width, top, bottom, height);
    //LOG_INFO("Window_draw loop %s %p rect     rect.x:%d, rect.y:%d, rect.width:%d, rect.height:%d", current->id, current, rect.x, rect.y, rect.width, rect.height);
    // LOG_INFO("parent %s", w->id);
    int skip = 0;
    // if (top < 0) skip = 1;
    if (geo.height <= top)
      skip = 1;
    // if (geo.height < bottom) skip = 1;
    if (height == 1 && top < 0)
      skip = 1;
    if (left < 0)
      skip = 1;
    if (geo.width < left)
      skip = 1;
    // if ((!(height == 1 && top < 0)) && top < geo.height)
    if (!skip)
      current->draw(current, hasFocus || current == draggingY);
    current = current->next;
  }
  // printf("\033[0m");
  // fflush(stdout);
}



Window *Window_init(Window *w, int left, int right, int top, int bottom, int width, int height)
{
  // Window* w = malloc(sizeof *w);
  w->head = NULL;
  w->tail = NULL;
  w->next = NULL;
  w->prev = NULL;

  w->left = left;
  w->right = right;
  w->top = top;
  w->bottom = bottom;
  // w->x = x;
  // w->y = y;
  w->width = width;
  w->height = height;
  w->virtual_height = height;
  // w->draw = draw;
  w->draw = Window_draw;
  w->set_top = Window_set_top;
  w->send_key = NULL;
  w->on_hover = NULL;
  w->undo_on_hover = NULL;
  w->parent = NULL;
  w->hidden = 0;
  w->shift = 0;

  w->c = NULL;


  return w;
}


int Geometry_in_bounds(Geometry geo, int x, int y)
{
  if (geo.x <= x && x < geo.x + geo.width &&
      geo.y <= y && y < geo.y + geo.height)
  {
    return 1;
  }
  return 0;
}

Window *Window_find_widget(struct Window *this, int x, int y)
{
  Geometry geo = this->calculated;
  // Window* current = w->head;
  if (!this)
    return NULL;
  if (this->hidden == 1){
    return NULL;
  }

  Window *ret = NULL;
  // LOG_INFO(" Window_find_widget %p %d %d %d %d %d %d", this, geo.x, geo.y, geo.width, geo.height, x, y);
  if (Geometry_in_bounds(geo, x, y))
  {
    // LOG_INFO("in bounds");
    ret = this;
  }
  Window *current = this->tail;
  while (current != NULL)
  {
    int skip = 0;

    /* Skip widgets that are outside parent's visible area (scrolled out of view) */
    if (current->parent) {
      Geometry parent_geo = current->parent->calculated;
      Geometry child_geo = current->calculated;

      // Check if child is completely outside parent's bounds
      if (child_geo.y >= parent_geo.y + parent_geo.height) {
        skip = 1; // below parent's bottom
      }
      if (child_geo.height == 1 && child_geo.y < parent_geo.y) {
        skip = 1; // single-line widget above parent's top
      }
      if (child_geo.y + child_geo.height <= parent_geo.y) {
        skip = 1; // completely above parent's top
      }
    }

    //if (strcmp(this->id, "slider") == 0) LOG_INFO("slider: %d", skip);
    //LOG_INFO("wskip: %s %d", this->id, skip);

    if (!skip) {
      Window *found = Window_find_widget(current, x, y);
      if (found != NULL)
      {
        return found;
      }
    }

    current = current->prev;
  }
  return ret;
}

void Window_bring_to_bottom(Window *this)
{
  LOG_INFO("Window_bring_to_bottom: %p", (void *)this->parent);
  if (!this || !this->parent)
    return;

  Window *parent = this->parent;

  // If already the tail, nothing to do
  if (parent->tail == this)
    return;

  // ---- 1. Unlink from current position ----
  if (this->prev)
  {
    this->prev->next = this->next;
  }
  else
  {
    // this was head
    parent->head = this->next;
  }

  if (this->next)
  {
    this->next->prev = this->prev;
  }
  else
  {
    // this was tail (redundant check, but safe)
    parent->tail = this->prev;
  }

  // ---- 2. Insert at tail ----
  this->next = NULL;
  this->prev = parent->tail;

  if (parent->tail)
  {
    parent->tail->next = this;
  }

  parent->tail = this;

  // If list was empty or had one element
  if (parent->head == NULL)
  {
    parent->head = this;
  }
}

/* widget.c */



void Widget_draw(struct Window *current, int hasFocus)
{
  //Widget *current = (Widget *)wg;
  Window_draw((Window *)current, hasFocus);
  Geometry geo = current->calculated;
  if (current->hidden == 1){
    //LOG_INFO("wg->hidden");
    return;
  }

  // check if visible
  int visible = 1;
  Window *cursor = current;
  while (cursor->parent != root)
  {
    cursor = cursor->parent;
  }
  cursor = cursor->next;
  while (cursor != NULL)
  {
    if (cursor->left < geo.x && geo.x + geo.width < cursor->left + cursor->width &&
        cursor->top < geo.y && geo.y + geo.height < cursor->top + cursor->height)
    {
      // LOG_INFO("skipping Widget_draw");
      return;
    }
    // visible = 0;
    cursor = cursor->next;
  }
  // if (! visible) return;

  int fg = current->fg;
  int bg = current->bg;
  if (strcmp(current->id, "menu")!=0){
  if (bg >= 232 + 4 && !hasFocus)
    bg -= 4;
  if (bg == WINDOW_BAR_COLOR && !hasFocus)
    bg = 243;
  }
#ifdef USE_BUFFER
  //Buffer_print(&main_buf, geo.y + wg_y, geo.x + wg_x, wg_width, current->c, current->fg, current->bg);
  Buffer_print(&main_buf, geo.y, geo.x, geo.width, current->c, fg, bg);
#else


  Buffer_print_raw(&main_buf, geo.y, geo.x, geo.width, current->c, fg, bg);
  // fprintf(stdout, "\033[0m");
  // fflush(stdout);
#endif
}

Window *Window_add_widget(Window *w, int left, int right, int top, int bottom, int width, int height, char *c, int fg, int bg)
{
  Window *wg = malloc(sizeof *wg);
  Window_init(wg, left, right, top, bottom, width, height);
  wg->draw = Widget_draw;
  wg->parent = w;
  wg->c = c;
  wg->id = c;
  wg->fg = fg;
  wg->bg = bg;
  // wg->_fg = 0;
  // wg->_bg = 0;

  Window_append(w, wg);

  w->virtual_height = max(w->virtual_height, top+height);

  return wg;
}

/*void Window_add_label(Window *parent, Window *child, char *c, int fg, int bg)
{
  //Window *wg = malloc(sizeof *wg);
  //Window_init(wg, -1, -1, -1, -1, -1, -1);
  child->draw = Widget_draw;
  child->parent = parent;
  child->c = c;
  child->id = c;
  child->fg = fg;
  child->bg = bg;

  Window_append(parent, child);

  parent->virtual_height = max(parent->virtual_height, 1);

}*/
const char *filename_from_path(const char *path)
{
    if (path == NULL || *path == '\0')
        return path;

    const char *slash1 = strrchr(path, '/');
    const char *slash2 = strrchr(path, '\\');  // Windows paths

    const char *last = slash1;
    if (slash2 && (!last || slash2 > last))
        last = slash2;

    return last ? last + 1 : path;
}

void Window_set_id_from_path(Window *self, char * path){
  char * filename = filename_from_path(path);
  snprintf(self->id, ID_LENGTH*4, "📁%s", filename);
  if (calculate_width(self->id) >= ID_LENGTH) {
    char * end = char_at(self->id, ID_LENGTH-3);
    end[0] = '.';
    end[1] = '.';
    end[2] = '.';
    end[3] = '\0';
  }
}

void Window_execute_lambda(struct Window *w, int x, int y)
{
    invoke_lambda(w->lambda);
}
