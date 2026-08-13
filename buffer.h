#ifndef BUFFER_H
#define BUFFER_H

#include <stdio.h>
#include <stdint.h>

#define USE_BUFFER


typedef struct Buffer
{
  int width;
  int height;
  uint32_t *buffer;
  unsigned char *bg;
  unsigned char *fg;
  uint32_t *buffer2;
  unsigned char *bg2;
  unsigned char *fg2;
} Buffer;


void Buffer_init(Buffer *buf, int width, int height);
void Buffer_clear(Buffer *buf);
void Buffer_print_raw(Buffer *buf, int y, int x, int width, char *s, int fg, int bg);
void Buffer_print(Buffer *buf, int y, int x, int width, char *s, int fg, int bg);
void Buffer_set_fg(Buffer *buf, int y, int x, int width, int fg);
void Buffer_set_bg(Buffer *buf, int y, int x, int width, int bg);
void Buffer_reset();
void Buffer_print_to_screen(Buffer *buf);
int calculate_width(char *s);
char * char_at(char *s, int i);
char * char_at_prev(char *s, int i);
int get_idx_pos(char *s, int i);
int count_chars(char *s);
uint32_t utf8_decode(const uint8_t **s);
int cp_width(int cp);


extern Buffer main_buf;

#endif
