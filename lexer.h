#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>

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
    TOK_NUMBER,          // integer or float
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
    LEX_SLASH_STAR_STAR
} LexerState;

typedef struct Token {
    size_t start; // included
    size_t end; // excluded
    TokenType type;
    int color;
} Token;

typedef struct Lexer {
    const char* source;
    size_t pos;
    //int loc;
    int state;           // for resuming complex lexing
    int finished;
    // ... other fields
} Lexer;

void lexer_init(Lexer* l, const char* source);
int lexer_next(Lexer* l, Token* out_token);

#endif
