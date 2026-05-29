#ifndef BUFFER_H
#define BUFFER_H

#include <stdio.h>
#include <stdint.h>

// #define USE_BUFFER


typedef struct Buffer
{
  int width;
  int height;
  uint32_t *buffer;
  char *bg;
  char *fg;
} Buffer;


void Buffer_init(Buffer *buf, int width, int height);
void Buffer_clear(Buffer *buf);
void Buffer_print_raw(Buffer *buf, int y, int x, int width, char *s, int fg, int bg);
void Buffer_print(Buffer *buf, int y, int x, int width, char *s, int fg, int bg);
void Buffer_reset();

extern Buffer main_buf;

#endif
