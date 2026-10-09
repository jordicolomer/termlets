#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

#include "common.h"
#include "mystr.h"

#ifdef _WIN32
#include <io.h>
#define STDOUT_FILENO 1
#define write _write

/* Basic wcwidth implementation for Windows */
static int mywcwidth(uint32_t wc) {
    /* Non-printable characters */
    // if (wc < 32 || (wc >= 0x7f && wc < 0xa0))
    if (wc < 32)
        return 0;

    /* Null character */
    // if (wc == 0)
    //     return 0;

    /* Combining characters (simplified) */
    // if (wc >= 0x0300 && wc <= 0x036F)
    //     return 0;

    /* Wide characters (CJK, emojis, etc.) */
    if ((wc >= 0x1100 && wc <= 0x115F) || /* Hangul Jamo */
        (wc >= 0x2E80 && wc <= 0x9FFF) || /* CJK */
        (wc >= 0xAC00 && wc <= 0xD7A3) || /* Hangul Syllables */
        (wc >= 0xF900 && wc <= 0xFAFF) || /* CJK Compatibility Ideographs */
        (wc >= 0xFE10 && wc <= 0xFE19) || /* Vertical forms */
        (wc >= 0xFE30 && wc <= 0xFE6F) || /* CJK Compatibility Forms */
        (wc >= 0xFF00 && wc <= 0xFF60) || /* Fullwidth Forms */
        (wc >= 0xFFE0 && wc <= 0xFFE6) || /* Fullwidth Forms */
        (wc >= 0x1F300 && wc <= 0x1F9FF)) /* Emojis */
        return 2;

    /* Default: single-width character */
    return 1;
}
#else
#include <unistd.h>
#endif

#include "ansi_term.h"
#include "logger.h"
#include "buffer.h"

Buffer2 buf2;

Buffer2 * buf0;
Buffer2 * buf1;

#include "collections.h"


ArrayList printList;

#define OUTBUF_SIZE (1024 * 1024 * 8 * 100)
char *out;


#include <stdint.h>
#include <stddef.h>

uint64_t hash_bytes(const void *data, size_t len)
{
    const uint8_t *p = data;
    uint64_t hash = 14695981039346656037ULL;

    for (size_t i = 0; i < len; i++) {
        hash ^= p[i];
        hash *= 1099511628211ULL;
    }

    return hash;
}

uint64_t hash_string(const char *s)
{
    uint64_t hash = 14695981039346656037ULL;

	if (s != NULL){
	  while (*s) {
        hash ^= (uint8_t)*s++;
        hash *= 1099511628211ULL;
	  }
	}

    return hash;
}


uint32_t utf8_decode2(const uint8_t *s, int *idx) {
    const uint8_t *p = s + *idx;
    uint32_t cp = 0;

    if (p[0] < 0x80) {
        cp = p[0];
        *idx += 1;
    } else if ((p[0] & 0xE0) == 0xC0) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *idx += 2;
    } else if ((p[0] & 0xF0) == 0xE0) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        *idx += 3;
    } else if ((p[0] & 0xF8) == 0xF0) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        *idx += 4;
    } else {
        *idx += 1;
    }

    return cp;
}

uint32_t utf8_decode_left2(const uint8_t *start, int *idx) {
    const uint8_t *s = start + *idx;
    const uint8_t *p = s;
    uint32_t cp = 0;

    if (p <= start)
        return 0;

    // Move to the first byte of the previous UTF-8 character.
    p--;
    (*idx)--;

    while (p > start && (p[0] & 0xC0) == 0x80) {
        p--;
        (*idx)--;
    }

    if (p[0] < 0x80) {
        cp = p[0];
    } else if ((p[0] & 0xE0) == 0xC0) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    } else if ((p[0] & 0xF0) == 0xE0) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    } else if ((p[0] & 0xF8) == 0xF0) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    } else {
        // Invalid UTF-8 leading byte.
        cp = 0;
    }

    //*s = p;

    return cp;
}

uint32_t utf8_decode(const uint8_t **s) {
    const uint8_t *p = *s;
    uint32_t cp = 0;

    if (p[0] < 0x80) {
        cp = p[0];
        *s += 1;
    } else if ((p[0] & 0xE0) == 0xC0) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *s += 2;
    } else if ((p[0] & 0xF0) == 0xE0) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        *s += 3;
    } else if ((p[0] & 0xF8) == 0xF0) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        *s += 4;
    } else {
        *s += 1;
    }

    return cp;
}

uint32_t utf8_decode_left(const uint8_t **s, const uint8_t *start) {
    const uint8_t *p = *s;
    uint32_t cp = 0;

    if (p <= start)
        return 0;

    // Move to the first byte of the previous UTF-8 character.
    p--;

    while (p > start && (p[0] & 0xC0) == 0x80)
        p--;

    if (p[0] < 0x80) {
        cp = p[0];
    } else if ((p[0] & 0xE0) == 0xC0) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    } else if ((p[0] & 0xF0) == 0xE0) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    } else if ((p[0] & 0xF8) == 0xF0) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    } else {
        // Invalid UTF-8 leading byte.
        cp = 0;
    }

    *s = p;

    return cp;
}

int utf8_encode(uint32_t cp, uint8_t **s) {
    uint8_t *p = *s;
    int ret = -1;

    if (cp < 0x80) {
        *p++ = (uint8_t)cp;
        ret = 1;
    } else if (cp < 0x800) {
        *p++ = 0xC0 | (uint8_t)(cp >> 6);
        *p++ = 0x80 | (uint8_t)(cp & 0x3F);
        ret = 2;
    } else if (cp < 0x10000) {
        *p++ = 0xE0 | (uint8_t)(cp >> 12);
        *p++ = 0x80 | (uint8_t)((cp >> 6) & 0x3F);
        *p++ = 0x80 | (uint8_t)(cp & 0x3F);
        ret = 3;
    } else if (cp < 0x110000) {
        *p++ = 0xF0 | (uint8_t)(cp >> 18);
        *p++ = 0x80 | (uint8_t)((cp >> 12) & 0x3F);
        *p++ = 0x80 | (uint8_t)((cp >> 6) & 0x3F);
        *p++ = 0x80 | (uint8_t)(cp & 0x3F);
        ret = 4;
    } else {
        // Invalid → encode U+FFFD
        *p++ = 0xEF;
        *p++ = 0xBF;
        *p++ = 0xBD;
        ret = -2;
    }

    *s = p;
    return ret;
}

Cell2 *cells_buffer1;
Cell2 *cells_buffer2;

Cell2 * get_cell(int y, int x){
  return &cells_buffer1[y * buf2.width + x];
}

Cell2 * get_cell2(int y, int x){
  return &cells_buffer2[y * buf2.width + x];
}

void swap_cells(){
  Cell2 *tmp = cells_buffer1;
  cells_buffer1 = cells_buffer2;
  cells_buffer2 = tmp;
}

void init_buf0(){
	memset(cells_buffer1, 0, sizeof(Cell2) * buf2.width * buf2.height);
}

void Buffer_init(Buffer *buf, int width, int height) {
    buf->width = width;
    buf->height = height;
    buf->buffer = calloc(width * height, sizeof(uint32_t));
    buf->bg = calloc(width * height, sizeof(char));
    buf->fg = calloc(width * height, sizeof(char));

    buf->buffer2 = calloc(width * height, sizeof(uint32_t));
    buf->bg2 = calloc(width * height, sizeof(char));
    buf->fg2 = calloc(width * height, sizeof(char));

    buf2.width = width;
    buf2.height = height;
    buf2.cells = calloc(width * height, sizeof(Cell));

	ArrayList_init(&printList, sizeof(PrintListElement), 10);
	cells_buffer1 = malloc(sizeof(Cell2)*width * height);
	cells_buffer2 = malloc(sizeof(Cell2)*width * height);

	// 3rd version
	buf0 = calloc(1, sizeof(Buffer2));
	buf0->width = width;
	buf0->height = height;
	buf0->cells = malloc(sizeof(Cell2)*width * height);	
	buf1 = calloc(1, sizeof(Buffer2));
	buf1->width = width;
	buf1->height = height;
	buf1->cells = malloc(sizeof(Cell2)*width * height);

	// all versions
	out = malloc(OUTBUF_SIZE);
}

void Buffer_clear(Buffer *buf) {
    int size = buf->width * buf->height;

    memset(buf->buffer, 0, size * 4);
    memset(buf->bg, BACKGROUND_COLOR, size);
    memset(buf->fg, 0, size);
}

int cp_width(int cp) {
    if (cp <= 31 && cp != '\t')
        return 0;
    if (cp == 173)
        return 0;
    if (cp == 133)
        return 0;
    if (32 < cp && cp < 255)
        return 1;
    if (cp == '\t')
        return 4;
    if (cp == 8991)
        return 1;
    if (cp == 8212)
        return 1;
    if (cp == 8211)
        return 1;
    if (cp == 9633)
        return 1;
    if (cp == 10550)
        return 1;
    if (cp == 9475)
        return 1;
    if (cp == 9650)
        return 1;
    if (cp == 9660)
        return 1;
    if (cp == 8595)
        return 1;
    if (cp == 128190)
        return 2;
    if (cp == 128316)
        return 2;
    if (cp == 128230)
        return 2;
    if (cp == 9211)
        return 1;
    // if (cp == 128444) return 2;
    if (cp == 128248)
        return 2;
    if (cp == 127966)
        return 2;
    // if (cp == 65279) return 0;
    // if (cp = 0x1F5BC) return 2;
    if (cp == 0x21A9)
        return 2;
    if (cp == 10004)
        return 1;

    if (9812 <= cp && cp <= 9823)
        return 1; // chess pieces
    // if (cp == 128221) return 2;
    // if (cp == 128444) return 2;
    // int width = mywcwidth(cp);
    // if (width == -1)
    //    return 1;
    if (cp >= 0x0300 && cp <= 0x036F)
        return 0;

    if ((cp >= 0x1100 && cp <= 0x115F) || /* Hangul Jamo */
        (cp >= 0x2E80 && cp <= 0x9FFF) || /* CJK */
        (cp >= 0xAC00 && cp <= 0xD7A3) || /* Hangul Syllables */
        (cp >= 0xF900 && cp <= 0xFAFF) || /* CJK Compatibility Ideographs */
        (cp >= 0xFE10 && cp <= 0xFE19) || /* Vertical forms */
        (cp >= 0xFE30 && cp <= 0xFE6F) || /* CJK Compatibility Forms */
        (cp >= 0xFF00 && cp <= 0xFF60) || /* Fullwidth Forms */
        (cp >= 0xFFE0 && cp <= 0xFFE6) || /* Fullwidth Forms */
        (cp >= 0x1F300 && cp <= 0x1F9FF)) /* Emojis */
        return 2;

    return 1;
}

char *char_at(char *s, int i, int *width) {
    const uint8_t *p = (const uint8_t *)s;
    const uint8_t *ant_p = (const uint8_t *)s;
    int total = 0;
    int idx = 0;
    int ant_idx = 0;

    while (*p) {
        if (idx == i) {
            if (width != NULL)
                *width = idx;
            return p;
        }
        if (idx > i) { // if multicell character, return the beginning of the character
            if (width != NULL)
                *width = ant_idx;
            return ant_p;
        }
        ant_p = p;
        ant_idx = idx;
        uint32_t cp = utf8_decode(&p);
        int w = cp_width(cp);
        // printf("%d %d\r\n", cp, w);

        idx += w;
    }
    return NULL;
}

char *char_at_prev(char *s, int i) {
    const uint8_t *p = (const uint8_t *)s;
    const uint8_t *prev = NULL;
    int total = 0;
    int idx = 0;

    while (*p) {
        if (idx >= i)
            return prev;
        prev = p; // Save pointer BEFORE advancing
        uint32_t cp = utf8_decode(&p);
        int w = cp_width(cp);
        // printf("%d %d\r\n", cp, w);

        idx += w;
    }
    return prev;
}

int calculate_width(char *s) {
    if (s == NULL)
        return 0;
    const uint8_t *p = (const uint8_t *)s;
    int total = 0;

    while (*p) {
        uint32_t cp = utf8_decode(&p);
        int w = cp_width(cp);
        // printf("%d %d\r\n", cp, w);

        total += w;
    }
	LOG_INFO("calculate_width %s %d", s, total);

    return total;
}


int calculate_width_n(char *s, size_t byte_len) {
    if (s == NULL)
        return 0;
    const uint8_t *p = (const uint8_t *)s;
    const uint8_t *end = p + byte_len;
    int total = 0;

    while (*p && p < end) {
        uint32_t cp = utf8_decode(&p);
        int w = cp_width(cp);
        total += w;
    }

    return total;
}

int y_state = -1;
int x_state = -1;
int fg_state = -1;
int bg_state = -1;

void Buffer_reset() {
    y_state = -1;
    x_state = -1;
    fg_state = -1;
    bg_state = -1;
}

void Buffer_print_raw(Buffer *buf, int y, int x, int width, char *s, int fg, int bg) {
    // LOG_INFO("Buffer_print_raw: %s y:%d x:%d width:%d fg:%d bg:%d", s, y, x, width, fg, bg);
    if (fg != fg_state || bg != bg_state) {
        set_color256(fg, bg);
        fg_state = fg;
        bg_state = bg;
    }
    if (x != x_state || y != y_state) {
        move_cursor(y, x);
        y_state = y;
        x_state = x;
    }
    // Calculate how many bytes to print to fit within width columns
    const uint8_t *p = (const uint8_t *)s;
    const uint8_t *start = p;
    int current_width = 0;
    int bytes_to_print = 0;

    while (*p) {
        const uint8_t *prev_p = p;
        uint32_t cp = utf8_decode(&p);
        int w = cp_width(cp);

        if (current_width + w <= width) {
            current_width += w;
            bytes_to_print = p - start;
        } else {
            break;
        }
    }

    // Print only the bytes that fit within width
    if (bytes_to_print > 0) {
        // printf("%.*s", bytes_to_print, s);
        // LOG_INFO("Buffer_print_raw: %s", s);
        write(STDOUT_FILENO, s, bytes_to_print);
    }

    // Pad remaining space
    for (int i = 0; i < width - current_width; i++)
        // printf(" ");
        write(STDOUT_FILENO, " ", 1);

    x_state += width;
}

void Buffer_print_raw_slow(Buffer *buf, int y, int x, int width, char *s, int fg, int bg) {
    // set_terminal_color(fg, bg);
    set_color256(fg, bg);
    move_cursor(y, x);
    if (width > 0) {
        for (int i = 0; i < width; i++)
            printf(" ");
    }
    move_cursor(y, x);
    printf("%s", s);
}

int count_chars(char *s) {
    const uint8_t *p = (const uint8_t *)s;
    int n = 0;
    while (*p) {
        uint32_t cp = utf8_decode(&p);
        n += 1;
    }
    return n;
}

int get_idx_pos(char *s, int i) {
    // given a utf-8 string and a char idx,
    // returns what position this char occupies on the screen
    const uint8_t *p = (const uint8_t *)s;
    int n = 0;
    int acc = 0;
    while (*p) {
        if (n == i)
            return acc;
        uint32_t cp = utf8_decode(&p);
        int w = cp_width(cp);
        acc += w;
        n += 1;
    }
    return acc;
}

void Buffer_print1(Buffer *buf, int y, int x, int width, char *s, int fg, int bg) {
    // x -= 1;
    y -= 1;
    const uint8_t *p = (const uint8_t *)s;

    for (int i = 0; i < width; i++) {
        buf->buffer[y * buf->width + x + i] = ' ';
        buf->fg[y * buf->width + x + i] = (char)fg;
        buf->bg[y * buf->width + x + i] = (char)bg;
    }
    int idx = 0;
    while (*p && idx < width) {
        uint32_t cp = utf8_decode(&p);
        // int w = wcwidth(cp);
        int w = cp_width(cp);
        // if (w == 0) idx -= 1;
        // LOG_INFO("Buffer_print %d %d %d\n", cp, x, y);
        if (cp == '\t') {
            // Tab: leave as spaces (from initialization), advance by tab_width
            if (show_tabs)
                buf->buffer[y * buf->width + x + idx] = 0x2192;
            idx += tab_width;
        } else {
            // Regular character: write it and mark continuation cells
            buf->buffer[y * buf->width + x + idx] = cp;
            // Mark continuation cells with -1 so they differ from regular spaces
            for (int i = 1; i < w; i++) {
                buf->buffer[y * buf->width + x + idx + i] = (uint32_t)-1;
            }
            idx += w;
        }
        // if (w == 0) idx += 1;
        // printf("U+%04X %d\n", cp, w);
    }
}

void Buffer_print2(Buffer *buf, int y, int x, int width, char *s, int fg, int bg) {
  //LOG_INFO("Buffer_print2 %s %d\n", s, width);
    y -= 1;
    for (int i = 0; i < width; i++) {
        buf2.cells[y * buf2.width + x + i].utf8 = " ";
        buf2.cells[y * buf2.width + x + i].size = 1;
        buf2.cells[y * buf2.width + x + i].fg = (char)fg;
        buf2.cells[y * buf2.width + x + i].bg = (char)bg;
        buf2.cells[y * buf2.width + x + i].width = 1;
    }
	//return;
    MyStr mystr;
    MyStr_init(&mystr, s);
    int offset = 0;
    while (MyStr_next_cluster(&mystr)) {
	  //if (mystr.pos_column + offset >= width)
	  if (mystr.pos_column + offset + mystr.width_column > width)
            break;

        if (mystr.width_codepoints == 1 && s[mystr.cluster_start] == '\t') {
            if (show_tabs) {
                Cell *cell = &buf2.cells[y * buf2.width + x + mystr.pos_column];
                cell->utf8 = "→";
                cell->size = strlen(cell->utf8);
                cell->width = 1;
            }
            // offset += tab_width - 1;

        } else {
		  if (mystr.width_column > 0){
            Cell *cell = &buf2.cells[y * buf2.width + x + mystr.pos_column + offset];
            cell->utf8 = s + mystr.cluster_start;
            cell->size = mystr.cluster_end - mystr.cluster_start;
            cell->width = mystr.width_column;
		  }
        }
    }
}

void Buffer_print3_buf(Buffer2 * buf, int y, int x, int width, char *s, int fg, int bg) {
  //LOG_INFO("Buffer_print2 %s %d\n", s, width);
    y -= 1;
    for (int i = 0; i < width; i++) {
	  Cell * cell = &buf->cells[y * buf->width + x + i];
        cell->utf8 = " ";
        cell->size = 1;
        cell->fg = (char)fg;
        cell->bg = (char)bg;
        cell->width = 1;
    }
	//return;
    MyStr mystr;
    MyStr_init(&mystr, s);
    int offset = 0;
    while (MyStr_next_cluster(&mystr)) {
        if (offset >= width)
            break;

        if (mystr.width_codepoints == 1 && s[mystr.cluster_start] == '\t') {
            if (show_tabs) {
                Cell *cell = &buf->cells[y * buf->width + x + mystr.pos_column];
                cell->utf8 = "→";
                cell->size = strlen(cell->utf8);
                cell->width = 1;
            }
            // offset += tab_width - 1;

        } else {
		  if (mystr.width_column > 0){
            Cell *cell = &buf->cells[y * buf->width + x + offset];
            cell->utf8 = s + mystr.cluster_start;
            cell->size = mystr.cluster_end - mystr.cluster_start;
            cell->width = mystr.width_column;
			offset += cell->width;
			/*
			cell->width = hashmap_get(cell->utf8, cell->size);
			if (cell->width != -1){
			  offset += cell->width;
			  for (int j=1;j<cell->width;j++) buf->cells[y * buf->width + x + offset + j].utf8 = (void *)-1;
			} else {
			  LOG_INFO("this should not happen \"%.*s\" %d", cell->size, cell->utf8, cell->width);
			  offset += 1;
			  }*/
		  }
        }
    }
}

static int get_cursor_position(int *row, int *col)
{
    char buf[32];
    int i = 0;

    printf("\033[6n");
    fflush(stdout);

    while (i < (int)sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            return -1;

        if (buf[i] == 'R') {
            i++;
            break;
        }

        i++;
    }

    if (i == 0 || i >= (int)sizeof(buf))
        return -1;

    buf[i] = '\0';

    if (sscanf(buf, "\033[%d;%dR", row, col) != 2)
        return -1;

    return 0;
}

int has_missing_width(int max_width, char *s) {
    MyStr mystr;
    MyStr_init(&mystr, s);
    int total_width = 0;
    while (MyStr_next_cluster(&mystr)) {
	  int len = mystr.cluster_end - mystr.cluster_start;
	  int width = hashmap_get(mystr.utf8 + mystr.cluster_start, len);
	  if (width == -1) return 1;
	  total_width+=width;
	  if (total_width >= max_width) break;
	}
	return 0;
  
}

#include <unistd.h>

void Buffer_print_and_cache(int y, int x, int max_width, char *s, int fg, int bg) {
  fprintf(stdout, "\033[%d;%dH", y, x+1);
  fprintf(stdout, "\033[38;5;%d;48;5;%dm", fg, bg);

  MyStr mystr;
  MyStr_init(&mystr, s);
  int total_width = 0;
  while (MyStr_next_cluster(&mystr)) {
	  int len = mystr.cluster_end - mystr.cluster_start;
	  char * data = mystr.utf8 + mystr.cluster_start;
	  int width = hashmap_get(data, len);
	  if (width == -1) {
		int row0, col0;
		get_cursor_position(&row0, &col0);
		//fflush(stdout);
		//usleep(10000);
		fprintf(stdout, "%.*s", (int)len, data);
		fflush(stdout);
		usleep(10000);
		int row1, col1;
		get_cursor_position(&row1, &col1);
		//fflush(stdout);
		//usleep(10000);
		width = col1 - col0;
		LOG_INFO("hashmap_put \"%.*s\" width:%d col0:%d col1:%d len:%d", (int)len, data, width, col0, col1, len);
		hashmap_put(data, len, width);
	  } else {
		LOG_INFO("write %d bytes", len);
		fprintf(stdout, "%.*s", (int)len, data);
	  }
	  
	  total_width+=width;
	  //LOG_INFO("%d >= %d", total_width, width);
	  if (total_width >= max_width) break;
	}  
}

void buf0_swap(){
  Buffer2 *tmp = buf0;
  buf0 = buf1;
  buf1 = tmp;
}

void buf0_reset(){
  memset(buf0->cells, 0, sizeof(Cell) * buf0->width * buf0->height);
}


void Buffer_print3(int y, int x, int width, char *s, int fg, int bg) {
  LOG_INFO("Buffer_print3 %s", s);
  if (has_missing_width(width, s)){
	LOG_INFO("has_missing_width");
	Buffer_print_and_cache(y, x, width, s, fg, bg);
	Buffer_print3_buf(buf1, y, x, width, s, fg, bg);
  } 
  Buffer_print3_buf(buf0, y, x, width, s, fg, bg);
}

void Buffer_print3_update(PrintListElement * elem, Cell2 * cells, int y, int x, int width, char *s, int fg, int bg) {
  uint64_t hash = hash_string(s);

  //x -= 1;
  for (int i=0;i<width;i++){
	Cell2 *cell = &cells[y * buf2.width + x + i];
	cell->hash = hash;
	cell->column = i;
	cell->bg = bg;
	cell->fg = fg;
  }
}

void Buffer_print_best(int y, int x, int width, char *s, int fg, int bg) {
  PrintListElement elem = { .y = y, .x = x, .width = width, .s = s, .fg = fg, .bg = bg };
  ArrayList_append(&printList, &elem);
  if (has_missing_width(width, s)){
	LOG_INFO("has_missing_width");
	Buffer_print_and_cache(y, x, width, s, fg, bg);
	Buffer_print3_update(&elem, cells_buffer1, y, x, width, s, fg, bg);
	}
}

void Buffer_print(Buffer *buf, int y, int x, int width, char *s, int fg, int bg) {
  //Buffer_print1(buf, y, x, width, s, fg, bg);
  //Buffer_print2(buf, y, x, width, s, fg, bg);
  Buffer_print3_buf(buf0, y, x, width, s, fg, bg);
    //Buffer_print3(y, x, width, s, fg, bg);
  //Buffer_print_best(y, x, width, s, fg, bg);
}

void Buffer_set_fg(Buffer *buf, int y, int x, int width, int fg) {
    y -= 1;
    for (int i = 0; i < width; i++) {
        buf->fg[y * buf->width + x + i] = (char)fg;
        buf2.cells[y * buf->width + x + i].fg = (char)fg;
    }
}

void Buffer_set_bg(Buffer *buf, int y, int x, int width, int bg) {
    y -= 1;
    for (int i = 0; i < width; i++) {
        buf->bg[y * buf->width + x + i] = (char)bg;
        buf2.cells[y * buf->width + x + i].bg = (char)bg;
    }
}

void Buffer_print_to_screen_old(Buffer *buf) {
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
    for (int y = 0; y < buf->height; y++) {
        for (int x = 0; x < buf->width; x++) {

            int idx = y * buf->width + x;

            uint32_t cp = buf->buffer[idx];
            if (cp == 0)
                continue;

            char bg = buf->bg[idx];
            char fg = buf->fg[idx];

            // move cursor (ANSI is 1-based)
            if (x != terminal_x || y != terminal_y) {
                fprintf(stdout, "\033[%d;%dH", y + 1, x + 1);
                terminal_x = x;
                terminal_y = y;
            }

            // update color only if changed
            if (bg != current_bg || fg != current_fg) {
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
            if (len > 0) {
                fwrite(buf, sizeof(char), len, stdout);
                buf[len] = 0;
                // LOG_INFO("fwrite: %d %d %s", y, x, buf);
            }
            int w = wcwidth(cp);
            if (w > 1) {
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
    // LOG_INFO("Execution time: %f ms\n", cpu_time_used * 1000);
}


static inline void append_str(char *out, size_t *pos, const char *s) {
    while (*s)
        out[(*pos)++] = *s++;
}

static inline void append_bytes(char *out, size_t *pos, const char *s, int len) {
    memcpy(out + *pos, s, len);
    *pos += len;
}

static inline void append_fmt(char *out, size_t *pos, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int written = vsnprintf(out + *pos, OUTBUF_SIZE - *pos, fmt, args);

    va_end(args);

    if (written > 0)
        *pos += written;
}

void Buffer_copy_to_second_buffer(Buffer *buf) {
    int size = buf->width * buf->height;
    memcpy(buf->buffer2, buf->buffer, size * sizeof(uint32_t));
    memcpy(buf->bg2, buf->bg, size * sizeof(char));
    memcpy(buf->fg2, buf->fg, size * sizeof(char));
}

void _Buffer_print_to_screen(Buffer *buf) {
    int cursor_movement_count = 0;
    int color_count = 0;

    clock_t start = clock();

    char *out = malloc(OUTBUF_SIZE);
    memset(out, 0, OUTBUF_SIZE);
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

    for (int y = 0; y < buf->height; y++) {

        for (int x = 0; x < buf->width; x++) {
            // Skip cells already rendered as part of previous wide character
            if (terminal_y == y && x < terminal_x) {
                continue;
            }

            int idx = y * buf->width + x;

            uint32_t cp = buf->buffer[idx];

            // Skip wide character continuation cells in current buffer
            if (cp == (uint32_t)-1)
                continue;

            if (cp == 0)
                cp = 32;

            int bg = (int)buf->bg[idx];
            int fg = (int)buf->fg[idx];

            uint32_t cp2 = buf->buffer2[idx];

            // Convert 0 to space for comparison
            if (cp2 == 0)
                cp2 = 32;
            // Keep -1 as -1 so it differs from space (32) and triggers re-rendering

            int bg2 = (int)buf->bg2[idx];
            int fg2 = (int)buf->fg2[idx];
            if (cp == cp2 && bg == bg2 && fg == fg2)
                continue;

            // cursor movement only when needed
            if (x != terminal_x || y != terminal_y) {
                cursor_movement_count += 1;

                // append_fmt(out, &pos, "\033[%d;%dH", y + 1, x + 1);
                append_fmt(out, &pos, "\033[%d;%dH", y + 1, x);
                // LOG_INFO("append_fmt pos: %d %d", y, x);
                terminal_x = x;
                terminal_y = y;
            }

            // color update only when changed
            if (bg != current_bg || fg != current_fg) {
                color_count += 1;

                // append_fmt(out, &pos, "\033[%d;%dm", fg, bg);
                append_fmt(out, &pos, "\033[38;5;%d;48;5;%dm", fg, bg);

                // LOG_INFO("append_fmt color: %d %d", fg, bg);
                current_bg = bg;
                current_fg = fg;
            }

            // encode UTF-8
            uint8_t utf8[8] = {0};
            uint8_t *ptr = utf8;

            int len = utf8_encode(cp, &ptr);

            if (len > 0) {
                append_bytes(out, &pos, (char *)utf8, len);
            }

            int w = cp_width(cp);
            utf8[len] = 0;

            terminal_x += w;
        }
    }

    // reset formatting
    append_str(out, &pos, "\033[0m");

    // move cursor below UI
    append_fmt(out, &pos, "\033[%d;1H", buf->height + 1);

    write(STDOUT_FILENO, out, pos);

    free(out);
    Buffer_copy_to_second_buffer(buf);

    clock_t end = clock();

    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    LOG_INFO("Execution time: %f ms size:%d cursor_movement_count:%d color_count:%d",
             cpu_time_used * 1000, pos, cursor_movement_count, color_count);
}

void Buffer_print_to_screen2(Buffer *buf) {
    int cursor_movement_count = 0;
    int color_count = 0;

    clock_t start = clock();

    char *out = malloc(OUTBUF_SIZE);
    memset(out, 0, OUTBUF_SIZE);
    if (!out)
        return;

    size_t pos = 0;

    // synchronized output begin
    // append_str(out, &pos, "\033[?2026h");

    int current_bg = -1;
    int current_fg = -1;

    int terminal_x = -1;
    int terminal_y = -1;

    // hide cursor
    append_str(out, &pos, "\033[?25l");

    // clear screen
    // append_str(out, &pos, "\033[2J");

    for (int y = 0; y < buf->height; y++) {

        for (int x = 0; x < buf->width; x++) {
            // Skip cells already rendered as part of previous wide character
            // if (terminal_y == y && x < terminal_x) {
            //    continue;
            //}

            int idx = y * buf->width + x;

            char *utf8 = buf2.cells[idx].utf8;
            int bg = (int)buf2.cells[idx].bg;
            int fg = (int)buf2.cells[idx].fg;
            int size = (int)buf2.cells[idx].size;
            int width = (int)buf2.cells[idx].width;

            if (utf8 == NULL) {
                utf8 = " ";
                width = 1;
                size = 1;
            }
            if (utf8 == NULL)
                continue;

            // cursor movement only when needed
            //if (x != terminal_x || y != terminal_y)
			{
                cursor_movement_count += 1;

                // append_fmt(out, &pos, "\033[%d;%dH", y + 1, x + 1);
                // LOG_INFO("move %d %d", y + 1, x);
                append_fmt(out, &pos, "\033[%d;%dH", y + 1, x + 1);
                // LOG_INFO("append_fmt pos: %d %d", y, x);
                terminal_x = x;
                terminal_y = y;
            }

            // color update only when changed
            if (bg != current_bg || fg != current_fg) {
                color_count += 1;

                // append_fmt(out, &pos, "\033[%d;%dm", fg, bg);
                // LOG_INFO("color update %d %d", fg, bg);
                append_fmt(out, &pos, "\033[38;5;%d;48;5;%dm", fg, bg);

                // LOG_INFO("append_fmt color: %d %d", fg, bg);
                current_bg = bg;
                current_fg = fg;
            }

            // encode UTF-8
            if (size > 0) {
			  //LOG_INFO("append_bytes size: %d width: %d \"%.*s\"", size, width, size, utf8);
                append_bytes(out, &pos, (char *)utf8, size);
            }

            terminal_x += width;
            if (width > 1) {
                x += width - 1;
            }
        }
    }

    // reset formatting
    append_str(out, &pos, "\033[0m");

    // move cursor below UI
    append_fmt(out, &pos, "\033[%d;1H", buf->height + 1);

    // append_str(out, &pos, "\033[?2026l");

    write(STDOUT_FILENO, out, pos);
    // LOG_INFO("STDOUT_FILENO: %s", STDOUT_FILENO);

    // int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // write(fd, out, pos);
    // close(fd);

    free(out);
    // Buffer_copy_to_second_buffer(buf);
    // buf2.cells = calloc(buf->width * buf->height, sizeof(Cell));
    memset(buf2.cells, 0, buf2.width * buf2.height * sizeof(Cell));

    clock_t end = clock();

    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    //LOG_INFO("Execution time: %f ms size:%d cursor_movement_count:%d color_count:%d",
    //         cpu_time_used * 1000, pos, cursor_movement_count, color_count);

	ArrayList_reset(&printList);
}


int Cell_equals(Cell2 * cell1, Cell2 * cell2){
  if (cell1->hash != cell2->hash) return 0;
  if (cell1->column != cell2->column) return 0;
  if (cell1->bg != cell2->bg) return 0;
  if (cell1->fg != cell2->fg) return 0;
  return 1;
}

//char * s_trimmed = ltrim(elem->s, offset);
char * ltrim(char * s, int max_width){
  MyStr mystr;
  MyStr_init(&mystr, s);
  int total_width = 0;
  while (MyStr_next_cluster(&mystr)) {
	int len = mystr.cluster_end - mystr.cluster_start;
	int width = hashmap_get(mystr.utf8 + mystr.cluster_start, len);
	if (width == -1) width = 1;
	total_width+=width;
	if (total_width > max_width) return mystr.utf8 + mystr.cluster_start;
  }
  return NULL;
}

//int byte_size = get_width_size(s_trimmed, width);
int get_width_size(char * s, int max_width){
  MyStr mystr;
  MyStr_init(&mystr, s);
  int total_width = 0;
  int len;
  while (MyStr_next_cluster(&mystr)) {
	len = mystr.cluster_end - mystr.cluster_start;
	int width = hashmap_get(mystr.utf8 + mystr.cluster_start, len);
	if (width == -1) width = 1;
	total_width+=width;
	if (total_width >= max_width) return mystr.cluster_end;
  }
  return mystr.cluster_end;
}

void _emit_block(size_t * pos, PrintListElement * elem, int cell1_x, int y, int x){
  // print element with bounding box
  if (elem == NULL){
	//LOG_INFO("elem == NULL %d %d %d", y, cell1_x+1, x+1);
	append_fmt(out, pos, "\033[38;5;%d;48;5;%dm", 0, 0);
	append_fmt(out, pos, "\033[%d;%dH", y, cell1_x+1);
	append_bytes(out, pos, " ", 1);
  } else {
	//LOG_INFO("elem != NULL %d %d %d \"%s\" fg:%d bg:%d", y, cell1_x+1, x+1, elem->s, elem->fg, elem->bg);
	//LOG_INFO("offset");
	int offset = cell1_x - elem->x;
	//LOG_INFO("calling ltrim %s %d", elem->s, offset);
	char * s_trimmed = ltrim(elem->s, offset);
	//LOG_INFO("trimmed %s", s_trimmed);
	  
	int width = x - cell1_x;
	  
	append_fmt(out, pos, "\033[%d;%dH", y, cell1_x+1);
	append_fmt(out, pos, "\033[38;5;%d;48;5;%dm", elem->fg, elem->bg);
	for(int x=0;x<width;x++) append_bytes(out, pos, " ", 1); // optimization: this is not always necessary
	if (s_trimmed != NULL){
	  append_fmt(out, pos, "\033[%d;%dH", y, cell1_x+1);
				
	  int byte_size = get_width_size(s_trimmed, width);
	  //LOG_INFO("get_width_size %.*s byte_size=%d width=%d", byte_size, s_trimmed, byte_size, width);

	  append_fmt(out, pos, "%.*s", byte_size, s_trimmed);
	}
  }
}

void emit_block(size_t * pos, PrintListElement * elem, int cell1_x, int y, int x){
  //LOG_INFO("elem->s:%s, elem->x:%d, elem->y:%d, elem->fg:%d, elem->bg:%d, cell1_x:%d, y:%d, x:%d", elem->s, elem->x, elem->y, elem->fg, elem->bg, cell1_x, y, x);
  if (elem == NULL){
	append_fmt(out, pos, "\033[38;5;%d;48;5;%dm", 0, 0);
	append_fmt(out, pos, "\033[%d;%dH", y, cell1_x+1);
	append_bytes(out, pos, " ", 1);
  } else {
	/*int offset = cell1_x - elem->x;
	char * s_trimmed = ltrim(elem->s, offset);
	int width = x - cell1_x;
	append_fmt(out, pos, "\033[%d;%dH", y, cell1_x+1);
	append_fmt(out, pos, "\033[38;5;%d;48;5;%dm", elem->fg, elem->bg);
	for(int x=0;x<width;x++) append_bytes(out, pos, " ", 1); // optimization: this is not always necessary
	if (s_trimmed != NULL){
	  append_fmt(out, pos, "\033[%d;%dH", y, cell1_x+1);
	  int byte_size = get_width_size(s_trimmed, width);
	  append_fmt(out, pos, "%.*s", byte_size, s_trimmed);
	  }*/
	MyStr mystr;
	MyStr_init(&mystr, elem->s);
	append_fmt(out, pos, "\033[38;5;%d;48;5;%dm", elem->fg, elem->bg);
	int target_x = elem->x;
	//if (*elem->s != 0){
	while (MyStr_next_cluster(&mystr)) {
	  if (cell1_x <= target_x && target_x < x){
		append_fmt(out, pos, "\033[%d;%dH", y, target_x+1);
		int byte_size = mystr.cluster_end - mystr.cluster_start;
		append_fmt(out, pos, "%.*s", byte_size, elem->s + mystr.cluster_start);
	  }
	  target_x += mystr.width_column;
    }
	//}
	if (target_x < cell1_x){
	  target_x = cell1_x;
	}
	append_fmt(out, pos, "\033[%d;%dH", y, target_x+1);
	while (target_x < x){
	  //LOG_INFO("target_x:%d x:%d", target_x, x);
	  append_fmt(out, pos, " ");
	  target_x+=1;
	}
  }
}

char * Buffer_print_to_screen_impl(Buffer *buf) {
  // best version
    int cursor_movement_count = 0;
    int color_count = 0;

    clock_t start = clock();

    //char *out = malloc(OUTBUF_SIZE);
    memset(out, 0, OUTBUF_SIZE);
    if (!out)
        return NULL;

    size_t pos = 0;
	
	init_buf0();
	
	// iterate list of blocks and construct buffer
	ArrayListIterator current = printList.first;
	while (ArrayListIteratorValid(&printList, &current)) {
	  PrintListElement *elem = ArrayListIteratorElement(&printList, &current);
	  uint64_t hash = hash_string(elem->s); // this should be done before

	  for(int column = 0 ; column < elem->width; column++){
		Cell2 * cell = get_cell(elem->y, elem->x + column);
		cell->hash = hash;
		cell->column = column;
		cell->elem = elem;
		cell->fg = elem->fg;
		cell->bg = elem->bg;
	  }

	  ArrayListIteratorNext(&printList, &current);
	}
	Cell2 * cell1 = NULL;
	Cell2 * cell2 = NULL;
	int cell1_x = 0;
	int cell2_x = 0;
	int cell_differs = 0;
	for(int y=0;y<buf2.height;y++){
	  for(int x=0;x<buf2.width;x++){
		// todo: compare with second buffer. skip if all cells exist
		//LOG_INFO("get_cell: %d %d", y, x);
		Cell2 * cell = get_cell(y, x);
		Cell2 * cell2 = get_cell2(y, x);
		//printf("%d %d %d\n", y, x, Cell_equals(cell, cell2));
		//LOG_INFO("end get_cell %p %p", cell1, cell);
		if (cell1 == NULL) {
		  //printf("null\n");
		  cell1 = cell;
		  cell1_x = x;
		  cell_differs = 0;
		} else if (cell1->hash != cell->hash ||
				   cell1->column + x - cell1_x != cell->column ||
				   cell1->bg != cell->bg ||
				   cell1->fg != cell->fg
				   )
		  {
			//printf("else\n");
			// emit block cell1, cell2
			//if (y==3) LOG_INFO("emit block %d %d %d cell_differs=%d", y, cell1_x, x, cell_differs);
			if (cell_differs > 0){
			  //LOG_INFO("cell_differs %d %d %d", y, cell1_x, x);
			  //printf("emit %d %d %d\n", y, x, cell_differs);
			  PrintListElement * elem = cell1->elem;
			  emit_block(&pos, elem, cell1_x, y, x);
			}

			// start next block
			cell1 = cell;
			cell1_x = x;
			cell_differs = 0;
		}
		if (! Cell_equals(cell, cell2)) cell_differs++;
		//cell_differs++;

		//cell2 = cell;
		//cell2_x = x;
	  }
	}
	/*
	// mark visible blocks
	for(int y=0;y<buf2.height;y++){
	  for(int x=0;x<buf2.width;x++){
		Cell2 * cell1 = get_cell(y, x);
		Cell2 * cell2 = get_cell2(y, x);
		if (! Cell_equals(cell1, cell2)){
		  //LOG_INFO("Cell_equals %p %p", cell1, cell2);
		  if (cell1->elem != NULL){
			cell1->elem->visible = 1;
		  } else {
			append_fmt(out, &pos, "\033[%d;%dH", y, x+1);
			append_bytes(out, &pos, " ", 1);
		  }
		}
	  }
	}
	// iterate again and only print the visible blocks
	current = printList.first;
	while (ArrayListIteratorValid(&printList, &current)) {
	  PrintListElement *elem = ArrayListIteratorElement(&printList, &current);

	  if (elem->visible == 1){
		append_fmt(out, &pos, "\033[%d;%dH", elem->y, elem->x+1);
		append_fmt(out, &pos, "\033[38;5;%d;48;5;%dm", elem->fg, elem->bg);
		for(int x=0;x<elem->width;x++) append_bytes(out, &pos, " ", 1);
		append_fmt(out, &pos, "\033[%d;%dH", elem->y, elem->x+1);
		if (elem->s != NULL){
		  MyStr mystr;
		  MyStr_init(&mystr, elem->s);
		  while (MyStr_next_cluster(&mystr)) {
			int len = mystr.cluster_end - mystr.cluster_start;

			int width = hashmap_get(mystr.utf8 + mystr.cluster_start, len);
			//width = 1;
			if (width == -1){
			  // flush
			  write(STDOUT_FILENO, out, pos);
			  //fflush(stdout);
			  pos = 0;
			
			  int row0, col0;
			  get_cursor_position(&row0, &col0);
			  append_bytes(out, &pos, elem->s + mystr.cluster_start, len);
			
			  // flush
			  write(STDOUT_FILENO, out, pos);
			  //fflush(stdout);
			  pos = 0;
			
			  int row1, col1;
			  get_cursor_position(&row1, &col1);

			  width = col1 - col0;
			
			  hashmap_put(mystr.utf8 + mystr.cluster_start, len, width);
			} else {
			  append_bytes(out, &pos, elem->s + mystr.cluster_start, len);
			}
			//LOG_INFO("cluster: \"%.*s\" width=%d", (int)len, mystr.utf8 + mystr.cluster_start, width);

		  }
		}
	  }

	  ArrayListIteratorNext(&printList, &current);
	}
	*/

    // reset formatting
    append_str(out, &pos, "\033[0m");

    // move cursor below UI
    append_fmt(out, &pos, "\033[%d;1H", buf->height + 1);
	out[pos] = 0;

    // append_str(out, &pos, "\033[?2026l");

    //LOG_INFO("STDOUT:  \"%.*s\" ", (int)pos, out);

    // int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // write(fd, out, pos);
    // close(fd);

    //free(out);
    // Buffer_copy_to_second_buffer(buf);
    // buf2.cells = calloc(buf->width * buf->height, sizeof(Cell));
    memset(buf2.cells, 0, buf2.width * buf2.height * sizeof(Cell));

    clock_t end = clock();

    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    LOG_INFO("Execution time: %f ms size:%d cursor_movement_count:%d color_count:%d",
             cpu_time_used * 1000, pos, cursor_movement_count, color_count);

	ArrayList_reset(&printList);
	swap_cells();
	return out;
}


Buffer main_buf;
/*
1. Compute final-writer for every cell (O(n + S))  
final_writer[0..n-1] = none
for each draw d in original order:
    for i = L_d .. R_d:
        final_writer[i] = d

(S = total length of all strings)

2. Compute the difference set (O(n))  
D = { i | prev[i] ≠ next[i] }

(You can obtain next either by a separate simulation or on the fly while building the final-writer array.)

3. Forced-set closure (O(n + S))  
forced = empty set          // set of draw indices
queue  = empty queue

// seed with the writers that are required by the real differences
for each i in D:
    w = final_writer[i]
    if w is not marked forced:
        mark w forced
        enqueue w

// propagate collateral damage
while queue is not empty:
    d = dequeue
    for i = L_d .. R_d:               // scan the interval once
        w = final_writer[i]
        if w is not marked forced:
            mark w forced
            enqueue w

or recursively

forced = empty set

function force(d):
    if d is already marked forced:
        return
    mark d forced
    for i = L_d .. R_d:                  // scan the interval
        w = final_writer[i]
        force(w)                         // recursive call

# seed
for each i in D:
    force(final_writer[i])


4. Emit
Print the forced draws in the order they originally appeared.
Their number is the exact minimum.

 */

int buf0_Cell_equals(Cell * cell1, Cell * cell2){
  if (cell1->size != cell2->size) return 0;
  if (cell1->bg != cell2->bg) return 0;
  if (cell1->fg != cell2->fg) return 0;
  if (memcmp(cell1->utf8, cell2->utf8, cell1->size) != 0) return 0;
  return 1;
}

void Buffer_print_to_screen___(Buffer *buf) {
    int cursor_movement_count = 0;
    int color_count = 0;

    clock_t start = clock();

    //char *out = malloc(OUTBUF_SIZE);
    memset(out, 0, OUTBUF_SIZE);
    if (!out)
        return;

    size_t pos = 0;

    // synchronized output begin
    // append_str(out, &pos, "\033[?2026h");

    int current_bg = -1;
    int current_fg = -1;

    int terminal_x = -1;
    int terminal_y = -1;

    // hide cursor
    append_str(out, &pos, "\033[?25l");

    // clear screen
    // append_str(out, &pos, "\033[2J");

    for (int y = 0; y < buf->height; y++) {

        for (int x = 0; x < buf->width; x++) {
            // Skip cells already rendered as part of previous wide character
            // if (terminal_y == y && x < terminal_x) {
            //    continue;
            //}

            int idx = y * buf->width + x;
			Cell * cell0 = &buf0->cells[idx];
			Cell * cell1 = &buf1->cells[idx];
			if (buf0_Cell_equals(cell0, cell1)) continue;

            char *utf8 = cell0->utf8;
            int bg = (int)cell0->bg;
            int fg = (int)cell0->fg;
            int size = (int)cell0->size;
            int width = (int)cell0->width;

            if (utf8 == -1)
                continue;
            if (utf8 == NULL) {
                utf8 = " ";
                width = 1;
                size = 1;
            }

            // cursor movement only when needed
            //if (x != terminal_x || y != terminal_y)
			{
                cursor_movement_count += 1;

                // append_fmt(out, &pos, "\033[%d;%dH", y + 1, x + 1);
                // LOG_INFO("move %d %d", y + 1, x);
                append_fmt(out, &pos, "\033[%d;%dH", y + 1, x);
                // LOG_INFO("append_fmt pos: %d %d", y, x);
                terminal_x = x;
                terminal_y = y;
            }

            // color update only when changed
            if (bg != current_bg || fg != current_fg) {
                color_count += 1;

                // append_fmt(out, &pos, "\033[%d;%dm", fg, bg);
                // LOG_INFO("color update %d %d", fg, bg);
                append_fmt(out, &pos, "\033[38;5;%d;48;5;%dm", fg, bg);

                // LOG_INFO("append_fmt color: %d %d", fg, bg);
                current_bg = bg;
                current_fg = fg;
            }

            // encode UTF-8
            if (size > 0) {
			  LOG_INFO("append_bytes size: %d width: %d \"%.*s\"", size, width, size, utf8);
                append_bytes(out, &pos, (char *)utf8, size);
            }

            terminal_x += width;
            if (width > 1) {
                x += width - 1;
            }
        }
    }

    // reset formatting
    append_str(out, &pos, "\033[0m");

    // move cursor below UI
    append_fmt(out, &pos, "\033[%d;1H", buf->height + 1);

    // append_str(out, &pos, "\033[?2026l");

    write(STDOUT_FILENO, out, pos);
    // LOG_INFO("STDOUT_FILENO: %s", STDOUT_FILENO);

    // int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // write(fd, out, pos);
    // close(fd);

    //free(out);
    // Buffer_copy_to_second_buffer(buf);
    // buf2.cells = calloc(buf->width * buf->height, sizeof(Cell));
    //memset(buf2.cells, 0, buf2.width * buf2.height * sizeof(Cell));

    clock_t end = clock();

    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    //LOG_INFO("Execution time: %f ms size:%d cursor_movement_count:%d color_count:%d",
    //         cpu_time_used * 1000, pos, cursor_movement_count, color_count);

	//ArrayList_reset(&printList);
	buf0_swap();
	buf0_reset();
}

void Buffer_print_to_screen(Buffer *buf) {
  Buffer_print_to_screen___(buf);
  //Buffer_print_to_screen2(buf);
  //Buffer_print_to_screen_impl(buf);
  
  /*char * out = Buffer_print_to_screen_impl(buf);
  //write(STDOUT_FILENO, out, pos);
  write(STDOUT_FILENO, out, strlen(out));*/
}
