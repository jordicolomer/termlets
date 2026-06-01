#ifndef WINDOW_H
#define WINDOW_H

#define WINDOW_BAR_COLOR 20


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
  // int x;
  // int y;
  int left;
  int right;
  int top;
  int bottom;
  int width;
  int height;
  // void (*draw)(struct Window*, int, int, int);
  void (*draw)(struct Window *, Geometry, int);
  void (*on_mouse_down)(struct Window *wg, int x, int y);
  void (*on_hover)(struct Window *wg, int x, int y);
  void (*undo_on_hover)(struct Window *wg, int x, int y);
  void (*set_top)(struct Window *wg, int top);
  void (*send_key)(struct Window *wg, char c);
  void *data;
  void *data2;
  int hidden;
  int shift;
} Window;

void Window_set_top(struct Window *wg, int top);

extern Window *root;

extern Window *dragging;
extern Window *resizing;
extern Window *focused;
extern Window *hovering;
extern Window *open_menu;
extern int dragging_offset_x, dragging_offset_y;

void Window_append(Window *w, Window *new_w);
void Window_remove(Window *w);
int Window_get_height(Window* wg);
void Window_draw(struct Window *w, Geometry geo, int hasFocus);
Window *Window_init(Window *w, int left, int right, int top, int bottom, int width, int height);
int Geometry_in_bounds(Geometry geo, int x, int y);
Window *Window_find_widget(struct Window *this, Geometry geo, int x, int y);
void Window_bring_to_bottom(Window *this);

/* widget.c */

typedef struct Widget
{
  char *id;
  // for now we copy from Window
  struct Window *parent;
  struct Window *head;
  struct Window *tail;
  struct Window *next;
  struct Window *prev;
  // int x;
  // int y;
  int left;
  int right;
  int top;
  int bottom;
  int width;
  int height;
  // void (*draw)(struct Window*, int, int, int);
  void (*draw)(struct Window *, Geometry, int);
  void (*on_mouse_down)(struct Widget *wg, int x, int y);
  void (*on_hover)(struct Window *wg, int x, int y);
  void (*undo_on_hover)(struct Window *wg, int x, int y);
  void (*set_top)(struct Window *wg, int top);
  void (*send_key)(struct Window *wg, char c);
  void *data;
  void *data2;
  int hidden;
  int shift;
  // for now we copy from Window

  char *c;
  int fg;
  int bg;
  // int _fg;
  // int _bg;
} Widget;

void Widget_draw(struct Window *wg, Geometry geo, int hasFocus);
Widget *Window_add_widget(Window *w, int left, int right, int top, int bottom, int width, int height, char *c, int fg, int bg);

#endif
