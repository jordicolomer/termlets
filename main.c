#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "ansi_term.h"

#ifdef _WIN32
    #define NOMINMAX  /* Prevent Windows from defining min/max macros */
    #include <windows.h>
    #include <conio.h>
    #include <io.h>
    #include <fcntl.h>
    #define STDIN_FILENO 0
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/select.h>
#endif
#include "logger.h"
#include "buffer.h"
#include "window.h"
#include "frame.h"
#include "file_manager.h"
#include "taskbar.h"
#include "utils.h"
#include "vterm_terminal.h"
#include "tabs.h"
#include "common.h"
#include "config.h"

// TERMINAL

void init()
{
  load_mappings();
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
  //LOG_INFO("repaint");
  //printf("\x1b[44m\x1b[37m");

  // redraw everything
#ifdef USE_BUFFER
  Buffer_clear(&main_buf);
#else
  set_color256(232, 17);
  clear_screen();
  hide_cursor();
#endif

  Geometry rect = {0, 0, root->width, root->height};
  root->calculated = rect;
  root->draw(root, 0);
  

#ifdef USE_BUFFER
  Buffer_print_to_screen(&main_buf);
#else
  //Reset all text attributes to terminal defaults.
  fprintf(stdout, "\033[0m");
  fflush(stdout);
#endif
  Buffer_reset();
}


 

void on_drag(int x, int y)
{
  // LOG_INFO("on_drag x: %d y: %d", x, y);
  if (draggingY != NULL)
  {
    //LOG_INFO("dragging");
    if (draggingX != NULL)
      draggingX->left = min(max(0, x - dragging_offset_x), draggingX->parent->width - draggingX->width);
    int parent_height = draggingY->parent->calculated.height;
    //LOG_INFO("on_drag %d %d", parent_height, dragging->parent->calculated.height);
    int new_top = min(max(0, y - dragging_offset_y), parent_height - draggingY->height);
    draggingY->set_top(draggingY, new_top);
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
    Window *wg = Window_find_widget(root, x, y);
    //LOG_INFO("Window_find_widget: %p id: %s", wg, wg->id);

    if (hovering != NULL && hovering->undo_on_hover != NULL && hovering != wg){
      hovering->undo_on_hover(hovering, x, y);
      repaint();
      hovering = NULL;
    }

    if (wg != NULL && wg->on_hover != NULL)
    {
      hovering = wg;
      wg->on_hover(wg, x, y);
      repaint();
    }
  }
}

void on_command_mouse_down(int x, int y)
{
  Window *wg = Window_find_widget(root, x, y);
  if (wg != NULL)
  {
    if (wg->on_command_mouse_down != NULL)
    {
      wg->on_command_mouse_down(wg, x, y);
    }
  }
  repaint();
}

void on_mouse_down(int x, int y)
{
  LOG_INFO("on_mouse_down: %d %d", x, y);
  //Geometry rect = {0, 0, root->width, root->height};
  Window *wg = Window_find_widget(root, x, y);
  LOG_INFO("Window_find_widget: %p", (void *)wg);
  Window * frame = Window_get_frame(wg);
  TaskBar_switch_frame(frame);

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
  draggingX = NULL;
  draggingY = NULL;
  resizing = NULL;
}

// #include <locale.h>
int start()
{
#ifdef _WIN32
  /* Set console to UTF-8 mode on Windows - MUST be done first */
  SetConsoleOutputCP(65001);  /* UTF-8 code page */
  SetConsoleCP(65001);         /* UTF-8 input code page */

  /* Set C runtime stdout/stderr to binary mode to prevent UTF-8 corruption */
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stderr), _O_BINARY);
#endif

  setlocale(LC_ALL, "");  /* Enable locale-aware character handling */

  log_init("app.log");

  // Enter alternate screen
  printf("\x1b[?1049h");
  enable_raw_mode();
  enable_mouse();
  //enter_alternate_screen();
  init();

  /* Start the PTY monitoring thread */
  start_pty_monitor_thread();

  int dragging = 0;
  repaint();

  while (1)
  {
    Window *focused_cursor = focused;
    if (focused_cursor != NULL) while (focused_cursor->send_key == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;

    /* Check if background thread signaled a repaint */
    if (check_and_clear_repaint_flag()) {
      repaint();
    }

    /* Use non-blocking read with timeout */
    if (!check_input_available(16666)) {  /* ~60fps */
      continue;  /* timeout or error, check repaint flag again */
    }

    /* Check if quit was requested from the menu */
    if (should_quit)
      break;

    char c;
    if (!read_char(&c)) {
      continue;
    }
    LOG_INFO("read %d", c);

    //if (c == 11){ // Ctrl+K
    if (c == '\t'){ // tab
      //cycle_task();
	  cycle_tab();
      repaint();
      continue;
    }
    if (c == 96){ // Ctrl+space
      cycle_task();
      repaint();
      continue;
    }
    if (c == 20){ // Ctrl+T
      vterminal_new();
      repaint();
      continue;
    }
    if (c == 5){ // Ctrl+E
      file_manager_new();
      repaint();
      continue;
    }
    /*if (insert_mode == 0){
      if (c == 'm'){
        cycle_tab();
        repaint();
        continue;
      }
      if (c == 'M'){
        cycle_tab_reverse();
        repaint();
        continue;
      }
	}*/
    if (c == 0){ // Ctrl+space then insert tab
	  c = '\t';
    }

    if (c != 27){
      if (focused_cursor != NULL) {
        if (focused_cursor->send_key != NULL){
          focused_cursor->send_key(focused_cursor, c);
          repaint();
          /* repaint will be triggered by PTY monitor thread */
        }
      }
    }


    // ESC sequence
    if (c == 27)
    {
      char seq[32];
      int i = 0;

      // read rest of escape sequence
      while (i < 31)
      {
        /* wait for more data with a short timeout */
        if (!check_input_available(100000)) {  /* 100ms timeout for escape sequences */
          /* timeout or error - no more data in escape sequence */
          break;
        }

        if (!read_char(&seq[i])) {
          break;
        }

        if (seq[i] == 'm' || seq[i] == 'M')
        {
          i++;
          break;
        }
        /* for arrow keys and other sequences, stop at a letter */
        if (i > 0 && seq[i] >= 'A' && seq[i] <= 'Z')
        {
          i++;
          break;
        }
        if (i > 0 && seq[i] >= 'a' && seq[i] <= 'z')
        {
          i++;
          break;
        }
        i++;
      }

      seq[i] = 0;

      LOG_INFO("seq: %s", seq);

      int btn, x, y;
      char type;

	  if (strcmp(seq, "[Z") == 0){
        cycle_tab_reverse();
        repaint();
        continue;
	  }


      if (sscanf(seq, "[<%d;%d;%d%c", &btn, &x, &y, &type) == 4)
      {
        //LOG_INFO("mouse event seq:%s btn:%d x:%d y:%d type:%c", seq, btn, x, y, type);
        /* mouse event */
        // LOG_INFO("sscanf %d %d %d %c", btn, x, y, type);
        if (dragging == 0 && btn == 0 && type == 'M')
        { // click
          // Don't allow starting drag from first row
          //if (y == 1)
          //  continue;
          dragging = 1;
          on_mouse_down(x, y);
        }
        if (/*dragging == 0 && */btn == 8 && type == 'M')
        { // click
          //dragging = 1;
          on_command_mouse_down(x, y);
        }
        if (dragging == 0 && btn == 64 && type == 'M')
        { // scroll wheel down 
		  Window *focused_cursor = focused;
		  if (focused_cursor != NULL) while (focused_cursor->scroll_wheel_up == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;
          if (focused_cursor != NULL && focused_cursor->scroll_wheel_up != NULL){
            focused_cursor->scroll_wheel_up(focused_cursor);
            repaint();
          }
        }
        if (dragging == 0 && btn == 65 && type == 'M')
        { // scroll wheel down 
		  Window *focused_cursor = focused;
		  if (focused_cursor != NULL) while (focused_cursor->scroll_wheel_up == NULL && focused_cursor->focused != NULL) focused_cursor = focused_cursor->focused;
          if (focused_cursor != NULL && focused_cursor->scroll_wheel_down != NULL){
            focused_cursor->scroll_wheel_down(focused_cursor);
            repaint();
          }
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
      else
      {
        /* not a mouse event, probably arrow keys or other keyboard sequence */
        if (focused_cursor != NULL && focused_cursor->send_sequence != NULL)
        {
          /* construct full escape sequence */
          //char full_seq[34];
          //full_seq[0] = 27;  /* ESC */
          //memcpy(full_seq + 1, seq, i);
		  //LOG_INFO("else %p %p %s", focused_cursor, focused_cursor->send_sequence, focused_cursor->id);
          //focused_cursor->send_sequence(focused_cursor, full_seq, i + 1);
          focused_cursor->send_sequence(focused_cursor, seq, i);
          repaint();
        }
      }
    }
  }

  cleanup();
  disable_mouse();
  disable_raw_mode();

   // Leave alternate screen
  /* Stop the PTY monitoring thread */
  stop_pty_monitor_thread();

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

#ifndef _WIN32
#include <execinfo.h>
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
#else
/* Windows crash handler - simplified version without backtrace */
void crash_handler(int sig)
{
  FILE *f = fopen("crash.log", "a");
  if (f) {
    fprintf(f, "\n--- Crash detected (signal %d) ---\n", sig);
    fclose(f);
  }
  _exit(1);
}
#endif
void setup_crash_handler()
{
  signal(SIGSEGV, crash_handler);
  signal(SIGABRT, crash_handler);
  signal(SIGFPE, crash_handler);
}

int parse_args(int argc, char **argv){
  //const char *config_file = NULL;
  //const char *filename = NULL;

  for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "--config") == 0) {
	  if (i + 1 >= argc) {
		fprintf(stderr, "--config requires a filename\n");
		return 1;
	  }
	  
	  config_file = argv[++i];
	}
	else if (strcmp(argv[i], "--help") == 0) {
	  printf("Usage: %s [options] [file]\n", argv[0]);
	  printf("  --config FILE    Use configuration file\n");
	  printf("  --help           Show this help\n");
	  return 0;
	}
	else if (argv[i][0] == '-') {
	  fprintf(stderr, "Unknown option: %s\n", argv[i]);
	  return 1;
	}
	/*else {
	  filename = argv[i];
	}*/
  }
  return 0;
}

int calculate_width(char *s);
int main(int argc, char **argv)
{
  parse_args(argc, argv);
  setup_crash_handler();
  start();
  //calculate_width("~$ll Stack Developer – Data Analytics and Integration.docx");
}