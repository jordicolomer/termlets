#ifndef WINDOW_H
#define WINDOW_H

#define WINDOW_BAR_COLOR 20
#include "lambda.h"

// this is the width
#define ID_LENGTH 20

typedef struct Geometry
{
  int x;
  int y;
  int width;
  int height;
} Geometry;

typedef struct Window
{
  char *id;
  struct Window *parent;
  struct Window *head;
  struct Window *tail;
  struct Window *next;
  struct Window *prev;
  struct Window *focused;
  // int x;
  // int y;
  int left;
  int right;
  int top;
  int bottom;
  int width;
  int height;
  int virtual_height;
  Geometry calculated;
  // void (*draw)(struct Window*, int, int, int);
  void (*draw)(struct Window *, int);
  void (*on_mouse_down)(struct Window *wg, int x, int y);
  void (*on_hover)(struct Window *wg, int x, int y);
  void (*undo_on_hover)(struct Window *wg, int x, int y);
  void (*set_top)(struct Window *wg, int top);
  void (*send_key)(struct Window *wg, char c);
  void (*send_sequence)(struct Window *wg, const char *seq, int len);
  void (*scroll_wheel_down)(struct Window *wg);
  void (*scroll_wheel_up)(struct Window *wg);
  void *data;
  void *data2;
  void *data3;
  int hidden;
  int shift;
  int shift_x;
  char *c;
  int fg;
  int bg;
  Lambda * lambda;
} Window;

void Window_set_top(struct Window *wg, int top);

extern Window *root;

extern Window *draggingX;
extern Window *draggingY;
extern Window *resizing;
extern Window *focused;
extern Window *hovering;
extern Window *open_menu;
extern int dragging_offset_x, dragging_offset_y;

void Window_append(Window *w, Window *new_w);
void Window_remove(Window *w);
//int Window_get_height(Window* wg);
void Window_draw(struct Window *w, int hasFocus);
Window *Window_init(Window *w, int left, int right, int top, int bottom, int width, int height);
int Geometry_in_bounds(Geometry geo, int x, int y);
Window *Window_find_widget(struct Window *this, int x, int y);
void Window_bring_to_bottom(Window *this);
void Window_set_id_from_path(Window *this, char * path);

/* widget.c */


void Widget_draw(struct Window *wg, int hasFocus);
Window *Window_add_widget(Window *w, int left, int right, int top, int bottom, int width, int height, char *c, int fg, int bg);
void Window_execute_lambda(struct Window *w, int x, int y);


#endif
