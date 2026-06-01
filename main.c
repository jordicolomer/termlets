#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include "ansi_term.h"
#include "logger.h"
#include "buffer.h"
#include "window.h"
#include "frame.h"
#include "file_manager.h"
#include "taskbar.h"

// TERMINAL

void init()
{
  int rows;
  int cols;
  get_terminal_size(&rows, &cols);
  LOG_INFO("get_terminal_size: %d %d", rows, cols);
  #ifdef USE_BUFFER
  Buffer_init(&main_buf, cols, rows);
  #endif

  root = malloc(sizeof *root);
  Window_init(root, 0, -1, 0, -1, cols, rows+1);
  root->id = "root";

  /*Window *w1 = FileExplorer_new(20, -1, 10, -1, 80, 20);
  w1->parent = root;
  w1->id = "FileExplorer1";
  Window_append(root, w1);

  Window *w2 = FileExplorer_new(30, -1, 15, -1, 80, 30);
  w2->parent = root;
  w2->id = "FileExplorer2";
  Window_append(root, w2);

  Window *w3 = FileExplorer_new(5, -1, 5, -1, 80, 30);
  w3->parent = root;
  w3->id = "FileExplorer3";
  Window_append(root, w3);*/

  TaskBar_new();

  /*Window* w2 = FileExplorer_test(100, 30, 80, 40);
  w2->parent = root;
  Window_append(root, w2);*/

  // dragging = w1;
}

void repaint()
{
  LOG_INFO("repaint");
  //printf("\x1b[44m\x1b[37m");
  //set_color256(232, 26);
  clear_screen();
  hide_cursor();

  // redraw everything
#ifdef USE_BUFFER
  Buffer_clear(&main_buf);
#endif

  Geometry rect = {0, 0, root->width, root->height};
  root->draw(root, rect, 0);
  
  //Reset all text attributes to terminal defaults.
  fprintf(stdout, "\033[0m");
  fflush(stdout);

#ifdef USE_BUFFER
  Buffer_print_to_screen(&main_buf);
#endif
  Buffer_reset();
}

int max(int a, int b)
{
  return (a > b) ? a : b;
}

int min(int a, int b)
{
  return (a < b) ? a : b;
}

 

void on_drag(int x, int y)
{
  // LOG_INFO("on_drag x: %d y: %d", x, y);
  if (dragging != NULL)
  {
    LOG_INFO("dragging");
    dragging->left = min(max(0, x - dragging_offset_x), dragging->parent->width - dragging->width);
    int parent_height = Window_get_height(dragging->parent);
    int new_top = min(max(0, y - dragging_offset_y), parent_height - dragging->height);
    dragging->set_top(dragging, new_top);
    repaint();
  }
  else if (resizing != NULL)
  {
    resizing->width = x - dragging_offset_x;
    resizing->height = y - dragging_offset_y;
    LOG_INFO("new dimensions width: %d height: %d", resizing->width, resizing->height);
    repaint();
  }
  else
  {
    Geometry rect = {0, 0, root->width, root->height};
    Widget *wg = Window_find_widget(root, rect, x, y);
    if (wg != NULL && wg->on_hover != NULL)
    {
      hovering = wg;
      wg->on_hover(wg, x, y);
      repaint();
    }
    else if (hovering != NULL && hovering->undo_on_hover != NULL)
    {
      hovering->undo_on_hover(hovering, x, y);
      repaint();
      hovering = NULL;
    }
  }
}

void on_mouse_down(int x, int y)
{
  LOG_INFO("on_mouse_down: %d %d", x, y);
  Geometry rect = {0, 0, root->width, root->height};
  Widget *wg = Window_find_widget(root, rect, x, y);
  LOG_INFO("Window_find_widget: %p", (void *)wg);

  int action_triggered = 0;

  if (wg != NULL)
  {
    LOG_INFO("wg id:%s wg->left:%d wg->right:%d wg->top:%d wg->bottom:%d wg->width:%d wg->height:%d", wg->id, wg->left, wg->right, wg->top, wg->bottom, wg->width, wg->height);
    if (wg->on_mouse_down != NULL)
    {
      LOG_INFO("wg->on_mouse_down");
      wg->on_mouse_down(wg, x, y);
      action_triggered = 1;
    }
    // dragging = wg;
  }
  if (!action_triggered && open_menu != NULL){
    open_menu->hidden = 1;
    open_menu = NULL;
  }
  repaint();
  //repaint();
}

void on_mouse_up()
{
  dragging = NULL;
  resizing = NULL;
}

// #include <locale.h>
int start()
{
  log_init("app.log");

  // Enter alternate screen
  printf("\x1b[?1049h");
  // setlocale(LC_ALL, "");
  enable_raw_mode();
  enable_mouse();
  //enter_alternate_screen();
  init();

  int dragging = 0;
  repaint();

  while (1)
  {
    char c;
    read(STDIN_FILENO, &c, 1);

    if (c == 'q')
      break;

    // ESC sequence
    if (c == 27)
    {
      char seq[32];
      int i = 0;

      // read rest of escape sequence
      while (i < 31 && read(STDIN_FILENO, &seq[i], 1) == 1)
      {
        if (seq[i] == 'm' || seq[i] == 'M')
        {
          i++;
          break;
        }
        i++;
      }

      seq[i] = 0;

      int btn, x, y;
      char type;

      if (sscanf(seq, "[<%d;%d;%d%c", &btn, &x, &y, &type) == 4)
      {
        if (y == 1)
          continue;
        // LOG_INFO("sscanf %d %d %d %c", btn, x, y, type);
        if (dragging == 0 && btn == 0 && type == 'M')
        { // click
          dragging = 1;
          on_mouse_down(x, y);
        }

        if (type == 'm')
        { // release
          dragging = 0;
          on_mouse_up();
        }

        // if (dragging == 1){
        on_drag(x, y);
        //}
      }
    }
  }

  cleanup();
  disable_mouse();
  disable_raw_mode();

   // Leave alternate screen
  printf("\x1b[?1049l");
  
  //set_color256(255, 0);
  //clear_screen();

  //printf("\033[2J\033[H");
  return 0;
}

/*
int test_buffer(){
  Buffer buf;
  Buffer_init(&buf, 20, 20);
  Buffer_print(&buf, 0, 0, 10, "🔪hello", RED_BG, BLACK);
  Buffer_print(&buf, 1, 0, 10, "🔪hi", BLUE_BG, BLACK);
  Buffer_print(&buf, 3, 3, 11, "hello world", WHITE_BG, BLACK);
  Buffer_print(&buf, 0, 4, 10, "hello", GREEN_BG, BLACK);
  Buffer_print_to_screen(&buf);
}
*/

#include <signal.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
void write_stacktrace_to_fd(int fd)
{
  void *array[50];
  int size = backtrace(array, 50);

  backtrace_symbols_fd(array, size, fd);
}

void crash_handler(int sig)
{
  int fd = open("crash.log",
                O_WRONLY | O_CREAT | O_APPEND,
                0644);

  if (fd >= 0)
  {
    dprintf(fd, "\n--- Crash detected (signal %d) ---\n", sig);
    write_stacktrace_to_fd(fd);
    close(fd);
  }

  _exit(1);
}
void setup_crash_handler()
{
  signal(SIGSEGV, crash_handler);
  signal(SIGABRT, crash_handler);
  signal(SIGFPE, crash_handler);
}

int main()
{
  setup_crash_handler();
  start();
  // test_buffer();
  // test_windows2();
}
