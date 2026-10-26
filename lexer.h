#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include "mystr.h"

enum Language { LANG_UNSELECT, LANG_NONE, LANG_C, LANG_CPP, LANG_JAVA, LANG_JS, LANG_TS, LANG_PY };

typedef enum {
    // End of input
    TOK_EOF,

    // Identifiers and literals
    TOK_PREPROC,
    TOK_COMMENT,
    TOK_LITERAL_NUMBER,
    TOK_LITERAL_STRING,
    TOK_LITERAL_STRING_ESCAPE,
    TOK_IDENTIFIER,
    TOK_FUNCTION,
    TOK_BRACKET,
    TOK_NUMBER, // integer or float
    TOK_STRING_LITERAL,
    TOK_CHAR_LITERAL
} TokenType;

typedef enum {
    LEX_BEGIN,
    LEX_BEGIN_HASH,
    LEX_NORMAL,
    LEX_LITERAL_NUMBER,
    LEX_LITERAL_STRING,
    LEX_LITERAL_STRING_ESCAPE,
    LEX_IDENTIFIER,
    LEX_SLASH,
    LEX_SLASH_SLASH,
    LEX_SLASH_STAR,
    LEX_SLASH_STAR_STAR,

    // Python multiline string states
    LEX_PY_QUOTE_DOUBLE_1,
    LEX_PY_QUOTE_DOUBLE_2,
    LEX_PY_QUOTE_SINGLE_1,
    LEX_PY_QUOTE_SINGLE_2,
    LEX_PYTHON_TRIPLE_DOUBLE,
    LEX_PYTHON_TRIPLE_SINGLE,
    LEX_PYTHON_TRIPLE_DOUBLE_1,
    LEX_PYTHON_TRIPLE_DOUBLE_2,
    LEX_PYTHON_TRIPLE_SINGLE_1,
    LEX_PYTHON_TRIPLE_SINGLE_2,
    LEX_LITERAL_STRING_SINGLE,
    LEX_LITERAL_STRING_SINGLE_ESCAPE
} LexerState;

typedef struct Token {
    size_t start;        // included
    size_t end;          // excluded
    size_t start_screen; // included
    size_t end_screen;   // excluded
    TokenType type;
    int color;
} Token;

typedef struct Lexer {
    const char *source;
    int reprocess;
    size_t pos;
    size_t pos_screen;
    // int loc;
    int state; // for resuming complex lexing
    int finished;
    int lang;
    char **keywords;
    char **types;
  MyStr mystr;
    // ... other fields
} Lexer;

void lexer_init(Lexer *l, const char *source, int lang);
int lexer_next(Lexer *l, Token *out_token);

#endif
