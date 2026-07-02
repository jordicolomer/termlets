#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>

#include "ansi_term.h"
#include "logger.h"
#include "buffer.h"

uint32_t utf8_decode(const uint8_t **s)
{
  const uint8_t *p = *s;
  uint32_t cp = 0;

  if (p[0] < 0x80)
  {
    cp = p[0];
    *s += 1;
  }
  else if ((p[0] & 0xE0) == 0xC0)
  {
    cp = ((p[0] & 0x1F) << 6) |
         (p[1] & 0x3F);
    *s += 2;
  }
  else if ((p[0] & 0xF0) == 0xE0)
  {
    cp = ((p[0] & 0x0F) << 12) |
         ((p[1] & 0x3F) << 6) |
         (p[2] & 0x3F);
    *s += 3;
  }
  else if ((p[0] & 0xF8) == 0xF0)
  {
    cp = ((p[0] & 0x07) << 18) |
         ((p[1] & 0x3F) << 12) |
         ((p[2] & 0x3F) << 6) |
         (p[3] & 0x3F);
    *s += 4;
  }
  else
  {
    *s += 1;
  }

  return cp;
}

int utf8_encode(uint32_t cp, uint8_t **s)
{
  uint8_t *p = *s;
  int ret = -1;

  if (cp < 0x80)
  {
    *p++ = (uint8_t)cp;
    ret = 1;
  }
  else if (cp < 0x800)
  {
    *p++ = 0xC0 | (uint8_t)(cp >> 6);
    *p++ = 0x80 | (uint8_t)(cp & 0x3F);
    ret = 2;
  }
  else if (cp < 0x10000)
  {
    *p++ = 0xE0 | (uint8_t)(cp >> 12);
    *p++ = 0x80 | (uint8_t)((cp >> 6) & 0x3F);
    *p++ = 0x80 | (uint8_t)(cp & 0x3F);
    ret = 3;
  }
  else if (cp < 0x110000)
  {
    *p++ = 0xF0 | (uint8_t)(cp >> 18);
    *p++ = 0x80 | (uint8_t)((cp >> 12) & 0x3F);
    *p++ = 0x80 | (uint8_t)((cp >> 6) & 0x3F);
    *p++ = 0x80 | (uint8_t)(cp & 0x3F);
    ret = 4;
  }
  else
  {
    // Invalid → encode U+FFFD
    *p++ = 0xEF;
    *p++ = 0xBF;
    *p++ = 0xBD;
    ret = -2;
  }

  *s = p;
  return ret;
}


void Buffer_init(Buffer *buf, int width, int height)
{
  buf->width = width;
  buf->height = height;
  buf->buffer = calloc(width * height, sizeof(uint32_t));
  buf->bg = calloc(width * height, sizeof(char));
  buf->fg = calloc(width * height, sizeof(char));

  buf->buffer2 = calloc(width * height, sizeof(uint32_t));
  buf->bg2 = calloc(width * height, sizeof(char));
  buf->fg2 = calloc(width * height, sizeof(char));
}

void Buffer_clear(Buffer *buf)
{
  int size = buf->width * buf->height;

  memset(buf->buffer, 0, size*4);
  memset(buf->bg, 0, size);
  memset(buf->fg, 0, size);
}

int cp_width(int cp)
{
  if (cp == 8991) return 1;
  if (cp == 8212) return 1;
  if (cp == 8211) return 1;
  if (cp == 9633) return 1;
  if (cp == 10550) return 1;
  //if (cp == 128221) return 2;
  //if (cp == 128444) return 2;
  int width = wcwidth(cp);
  if (width == -1)
    return 2;
  return width;
}

int calculate_width(char *s)
{
  const uint8_t *p = (const uint8_t *)s;
  int total = 0;

  while (*p)
  {
    uint32_t cp = utf8_decode(&p);
    int w = cp_width(cp);
    //printf("%d %d\r\n", cp, w);

    total += w;
  }

  return total;
}

int y_state = -1;
int x_state = -1;
int fg_state = -1;
int bg_state = -1;

void Buffer_reset(){
  y_state = -1;
  x_state = -1;
  fg_state = -1;
  bg_state = -1;
}

void Buffer_print_raw(Buffer *buf, int y, int x, int width, char *s, int fg, int bg)
{
  //LOG_INFO("Buffer_print_raw: %s y:%d x:%d width:%d fg:%d bg:%d", s, y, x, width, fg, bg);
  if (fg != fg_state || bg != bg_state)
  {
    set_color256(fg, bg);
    fg_state = fg;
    bg_state = bg;
  }
  if (x != x_state || y != y_state)
  {
    move_cursor(y, x);
    y_state = y;
    x_state = x;
  }
  // Calculate how many bytes to print to fit within width columns
  const uint8_t *p = (const uint8_t *)s;
  const uint8_t *start = p;
  int current_width = 0;
  int bytes_to_print = 0;

  while (*p)
  {
    const uint8_t *prev_p = p;
    uint32_t cp = utf8_decode(&p);
    int w = cp_width(cp);

    if (current_width + w <= width)
    {
      current_width += w;
      bytes_to_print = p - start;
    }
    else
    {
      break;
    }
  }

  // Print only the bytes that fit within width
  if (bytes_to_print > 0)
  {
    //printf("%.*s", bytes_to_print, s);
    //LOG_INFO("Buffer_print_raw: %s", s);
    write(STDOUT_FILENO, s, bytes_to_print);
  }

  // Pad remaining space
  for (int i = 0; i < width - current_width; i++)
    //printf(" ");
    write(STDOUT_FILENO, " ", 1);

  x_state += width;
}

void Buffer_print_raw_slow(Buffer *buf, int y, int x, int width, char *s, int fg, int bg)
{
  // set_terminal_color(fg, bg);
  set_color256(fg, bg);
  move_cursor(y, x);
  if (width > 0)
  {
    for (int i = 0; i < width; i++)
      printf(" ");
  }
  move_cursor(y, x);
  printf("%s", s);
}

void Buffer_print(Buffer *buf, int y, int x, int width, char *s, int fg, int bg)
{
  //x -= 1;
  y -= 1;
  const uint8_t *p = (const uint8_t *)s;

  for (int i = 0; i < width; i++)
  {
    buf->buffer[y * buf->width + x + i] = ' ';
    buf->fg[y * buf->width + x + i] = (char) fg;
    buf->bg[y * buf->width + x + i] = (char) bg;
  }
  int idx = 0;
  while (*p && idx < width)
  {
    uint32_t cp = utf8_decode(&p);
    //int w = wcwidth(cp);
    int w = cp_width(cp);
    //LOG_INFO("Buffer_print %d %d %d\n", cp, x, y);
    buf->buffer[y * buf->width + x + idx] = cp;
    // Mark continuation cells with -1 so they differ from regular spaces
    for (int i = 1; i < w; i++) {
       buf->buffer[y * buf->width + x + idx + i] = (uint32_t)-1;
    }
    //x += w;
    idx += w;
    // printf("U+%04X %d\n", cp, w);
  }
}
void Buffer_print_to_screen_old(Buffer *buf)
{
  clock_t start = clock();

  int current_bg = -1;
  int current_fg = -1;

  // clear screen + move cursor home
  // fprintf(stdout, "\033[2J\033[H");
  fprintf(stdout, "\033[2J");
  // fprintf(stdout, "\033[H");
  // LOG_INFO("buf->height buf->width: %d %d", buf->height, buf->width);
  write(STDOUT_FILENO, "\033[?25l", 6);

  int terminal_x = -1;
  int terminal_y = -1;
  for (int y = 0; y < buf->height; y++)
  {
    for (int x = 0; x < buf->width; x++)
    {

      int idx = y * buf->width + x;

      uint32_t cp = buf->buffer[idx];
      if (cp == 0)
        continue;

      char bg = buf->bg[idx];
      char fg = buf->fg[idx];

      // move cursor (ANSI is 1-based)
      if (x != terminal_x || y != terminal_y)
      {
        fprintf(stdout, "\033[%d;%dH", y + 1, x + 1);
        terminal_x = x;
        terminal_y = y;
      }

      // update color only if changed
      if (bg != current_bg || fg != current_fg)
      {
        // fprintf(stdout, "\033[%dm", color);
        set_terminal_color(bg, fg);
        current_bg = bg;
        current_fg = fg;
      }

      // char buf[10];
      // char * pointer = &buf;
      uint8_t buf[20] = {0}; // use uint8_t, give some margin
      uint8_t *ptr = buf;    // ← Correct
      int len = utf8_encode(cp, &ptr);
      // printf(" %d\n", len);
      if (len > 0)
      {
        fwrite(buf, sizeof(char), len, stdout);
        buf[len] = 0;
        // LOG_INFO("fwrite: %d %d %s", y, x, buf);
      }
      int w = wcwidth(cp);
      if (w > 1)
      {
        x += w - 1;
      }
      terminal_x += w;
    }
  }

  // reset formatting at end
  // fprintf(stdout, "\033[0m");

  fprintf(stdout, "\033[0m");
  fprintf(stdout, "\033[%d;1H", buf->height + 1);
  // fprintf(stdout, "\033[%d;1H", 20);
  fflush(stdout);
  clock_t end = clock();
  double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  LOG_INFO("Execution time: %f ms\n", cpu_time_used * 1000);
}

#define OUTBUF_SIZE (1024 * 1024 * 8)

static inline void append_str(char *out, size_t *pos, const char *s)
{
  while (*s)
    out[(*pos)++] = *s++;
}

static inline void append_bytes(char *out, size_t *pos, const char *s, int len)
{
  memcpy(out + *pos, s, len);
  *pos += len;
}

static inline void append_fmt(char *out, size_t *pos, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  int written = vsnprintf(out + *pos,
                          OUTBUF_SIZE - *pos,
                          fmt,
                          args);

  va_end(args);

  if (written > 0)
    *pos += written;
}

void Buffer_copy_to_second_buffer(Buffer *buf)
{
  /*for (int y = 0; y < buf->height; y++)
  {

    for (int x = 0; x < buf->width; x++)
    {

      int idx = y * buf->width + x;

      uint32_t cp = buf->buffer[idx];
      int bg = (int) buf->bg[idx];
      int fg = (int) buf->fg[idx];

      buf->buffer2[idx] = cp;
      buf->bg2[idx] = (char) bg;
      buf->fg2[idx] = (char) fg;
    }
  }*/
  int size = buf->width * buf->height;
  memcpy(buf->buffer2, buf->buffer, size * sizeof(uint32_t));
  memcpy(buf->bg2, buf->bg, size * sizeof(char));
  memcpy(buf->fg2, buf->fg, size * sizeof(char));
}

void Buffer_print_to_screen(Buffer *buf)
{
  int cursor_movement_count = 0;
  int color_count = 0;

  clock_t start = clock();

  char *out = malloc(OUTBUF_SIZE);
  if (!out)
    return;

  size_t pos = 0;

  // append_str(out, &pos, "\033[?2026h");

  int current_bg = -1;
  int current_fg = -1;

  int terminal_x = -1;
  int terminal_y = -1;

  // hide cursor
  append_str(out, &pos, "\033[?25l");

  // clear screen
  // append_str(out, &pos, "\033[2J");

  for (int y = 0; y < buf->height; y++)
  {

    for (int x = 0; x < buf->width; x++)
    {
      // Skip cells already rendered as part of previous wide character
      if (terminal_y == y && x < terminal_x) {
        continue;
      }

      int idx = y * buf->width + x;

      uint32_t cp = buf->buffer[idx];

      // Skip wide character continuation cells in current buffer
      if (cp == (uint32_t)-1)
        continue;

      if (cp == 0) cp = 32;

      int bg = (int) buf->bg[idx];
      int fg = (int) buf->fg[idx];

      uint32_t cp2 = buf->buffer2[idx];

      // Convert 0 to space for comparison
      if (cp2 == 0) cp2 = 32;
      // Keep -1 as -1 so it differs from space (32) and triggers re-rendering

      int bg2 = (int) buf->bg2[idx];
      int fg2 = (int) buf->fg2[idx];
      if (cp == cp2 && bg == bg2 && fg == fg2) continue;

      // cursor movement only when needed
      if (x != terminal_x || y != terminal_y)
      {
        cursor_movement_count += 1;

        //append_fmt(out, &pos, "\033[%d;%dH", y + 1, x + 1);
        append_fmt(out, &pos, "\033[%d;%dH", y+1, x);
        //LOG_INFO("append_fmt pos: %d %d", y, x);
        terminal_x = x;
        terminal_y = y;

      }

      // color update only when changed
      if (bg != current_bg || fg != current_fg)
      {
        color_count += 1;

        //append_fmt(out, &pos, "\033[%d;%dm", fg, bg);
        append_fmt(out, &pos, "\033[38;5;%d;48;5;%dm", fg, bg);
         
        //LOG_INFO("append_fmt color: %d %d", fg, bg);
        current_bg = bg;
        current_fg = fg;
      }

      // encode UTF-8
      uint8_t utf8[8] = {0};
      uint8_t *ptr = utf8;

      int len = utf8_encode(cp, &ptr);

      if (len > 0)
      {
        append_bytes(out,
                     &pos,
                     (char *)utf8,
                     len);
      }

      int w = cp_width(cp);
      utf8[len] = 0;
      //if (x == 27 && y == 31) 
      //LOG_INFO("Buffer_print_to_screen %s %d %d %d %d\n", utf8, cp, w, x, y);
      /*int w;

      if (cp < 128)
        w = 1;
      else
        w = cp_width(cp);

      if (w < 1)
        w = 1;*/

      // Don't modify loop variable - let the skip check at loop start handle it
      // if (w > 1)
      //   x += w - 1;

      terminal_x += w;
    }
  }

  // reset formatting
  append_str(out, &pos, "\033[0m");

  // move cursor below UI
  append_fmt(out,
             &pos,
             "\033[%d;1H",
             buf->height + 1);

  // show cursor again
  // append_str(out, &pos, "\033[?25h");
  // append_str(out, &pos, "\033[?2026l");
  //  ONE write
  write(STDOUT_FILENO, out, pos);

  //log_file = fopen(filename, "w");
  //fwrite(out, sizeof(char), pos, stdout);
  //out[pos] = 0;
  //LOG_INFO("%s\n", out);

  free(out);
  Buffer_copy_to_second_buffer(buf);

  clock_t end = clock();

  double cpu_time_used =
      ((double)(end - start)) / CLOCKS_PER_SEC;

  LOG_INFO("Execution time: %f ms size:%d cursor_movement_count:%d color_count:%d\n",
           cpu_time_used * 1000, pos, cursor_movement_count, color_count);
}

Buffer main_buf;
