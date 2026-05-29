#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "window.h"
#include "logger.h"
#include "buffer.h"


void Window_set_top(struct Window *wg, int top){
  wg->top = top;
}

Window *root = NULL;

Window *dragging = NULL;
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

  w->tail = new_w;
}

int Window_get_height(Window* wg){
  if (wg->height >= 0){
    return wg->height;
  }
  return Window_get_height(wg->parent) - wg->top - wg->bottom;
}

/*
int Window_get_width(Window* wg){
  if (wg->width >= 0){
  return wg->width;
  }
  return wg->parent->width - wg->left;
}

int Window_get_x(Window* wg){
  //LOG_INFO("Window_get_x");
  if (wg->left >= 0){
  //LOG_INFO("if");
  return wg->left;
  }
  //LOG_INFO("else");
  return wg->parent->width + wg->left;
}

int Window_get_y(Window* wg){
  if (wg-> >= 0){
  return wg->y;
  }
  return wg->parent->height + wg->y + 1;
}

int Window_get_absolute_x(Window* w){
  if (w == NULL) return 0;
  int x = Window_get_x(w);
  int x_parent = Window_get_absolute_x(w->parent);
  return x_parent+x;
}

int Window_get_absolute_y(Window* w){
  if (w == NULL) return 0;
  int y = Window_get_y(w);
  int y_parent = Window_get_absolute_y(w->parent);
  return y_parent+y;
}
*/

// void Window_draw(struct Window* w, int bias_x, int bias_y, int hasFocus){
void Window_draw(struct Window *w, Geometry geo, int hasFocus)
{
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
    int right = current->right;
    int width = current->width;

    if (left == -1)
      left = geo.width - current->right - current->width;
    if (right == -1)
      right = geo.width - current->left - current->width;
    if (width == -1)
      width = geo.width - current->left - current->right;

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

    Geometry rect = {geo.x + left, geo.y + top, width, height};
    // LOG_INFO("Window_draw loop %s %p orig css left:%d right:%d width:%d top:%d bottom:%d height:%d", current->id, current, current->left, current->right, current->width, current->top, current->bottom, current->height);
    // LOG_INFO("Window_draw loop %s %p comp css left:%d right:%d width:%d top:%d bottom:%d height:%d", current->id, current, left, right, width, top, bottom, height);
    // LOG_INFO("Window_draw loop %s %p rect     rect.x:%d, rect.y:%d, rect.width:%d, rect.height:%d", current->id, current, rect.x, rect.y, rect.width, rect.height);
    // LOG_INFO("parent %s", w->id);
    int skip = 0;
    // if (top < 0) skip = 1;
    if (geo.height <= top)
      skip = 1;
    // if (geo.height < bottom) skip = 1;
    if (height == 1 && top < 0)
      skip = 1;
    // if ((!(height == 1 && top < 0)) && top < geo.height)
    if (!skip)
      current->draw(current, rect, hasFocus || current == dragging);
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
  // w->draw = draw;
  w->draw = Window_draw;
  w->set_top = Window_set_top;
  w->on_hover = NULL;
  w->undo_on_hover = NULL;
  w->parent = NULL;
  w->hidden = 0;
  w->shift = 0;

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

Window *Window_find_widget(struct Window *this, Geometry geo, int x, int y)
{
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
    int left = current->left;
    int right = current->right;
    int width = current->width;

    if (left == -1)
      left = geo.width - current->right - current->width;
    if (right == -1)
      right = geo.width - current->left - current->width;
    if (width == -1)
      width = geo.width - current->left - current->right;

    int top = current->top;
    int bottom = current->bottom;
    int height = current->height;

    if (top == -1)
      top = geo.height - current->height - current->bottom;
    if (bottom == -1)
      bottom = geo.height - current->top - current->height;
    if (height == -1)
      height = geo.height - current->top - current->bottom;

    Geometry rect = {geo.x + left, geo.y + top, width, height};

    Window *found = Window_find_widget(current, rect, x, y);
    if (found != NULL)
    {
      return found;
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



void Widget_draw(struct Window *wg, Geometry geo, int hasFocus)
{
  Widget *current = (Widget *)wg;
  Window_draw((Window *)current, geo, hasFocus);
#ifdef USE_BUFFER
  Buffer_print(&main_buf, geo.y + wg_y, geo.x + wg_x, wg_width, current->c, current->fg, current->bg);
#else

  if (wg->hidden == 1){
    //LOG_INFO("wg->hidden");
    return;
  }

  // check if visible
  int visible = 1;
  Window *cursor = wg;
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
  if (bg >= 232 + 4 && !hasFocus)
    bg -= 4;
  if (bg == WINDOW_BAR_COLOR && !hasFocus)
    bg = 243;
  Buffer_print_raw(&main_buf, geo.y, geo.x, geo.width, current->c, fg, bg);
  // fprintf(stdout, "\033[0m");
  // fflush(stdout);
#endif
}

Widget *Window_add_widget(Window *w, int left, int right, int top, int bottom, int width, int height, char *c, int fg, int bg)
{
  Widget *wg = malloc(sizeof *wg);
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

  return wg;
}