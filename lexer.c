#include "lexer.h"
#include <string.h>

void lexer_init(Lexer* l, const char* source){
    l->source = source;
    l->pos = 0;
    l->state = 0;
    l->finished = 0;
}

/*
TOK_COMMENT
244
Dark Gray
Subtle, doesn't distract

TOK_KEYWORD
170
Purple / Magenta
Classic for keywords

TOK_IDENTIFIER
252
Light Gray
Neutral / default

TOK_NUMBER
81
Bright Cyan
Stands out nicely

TOK_STRING_LITERAL
114
Light Green
Easy on the eyes

TOK_CHAR_LITERAL
114
Light Green
Same as string

TOK_OPERATOR
207
Bright Pink/Magenta
Good visibility

TOK_PUNCTUATION
242
Gray
Subtle

TOK_UNKNOWN / Error
196
Bright Red
Error highlighting
*/

int isalpha(char c){
    return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
}

int isnumber(char c){
    return '0' <= c && c <= '9';
}

static const char *keywords[] = {
    "if",
    "else",
    "while",
    "for",
    "return",
    "switch",
    "",   // sentinel
};

static const char *types[] = {
    "int",
    "const",
    "char",
    "long",
    "short",
    "void",
    "",   // sentinel
};

int in_list(const char *c, int len, const char *kw[])
{
    for (int i = 0; keywords[i][0] != '\0'; i++) {
        if ((int)strlen(kw[i]) == len &&
            memcmp(c, kw[i], len) == 0) {
            return 1;
        }
    }

    return 0;
}

int is_keyword(const char *c, int len)
{
    return in_list(c, len, keywords);
}

int is_type(const char *c, int len)
{
    return in_list(c, len, types);
}

int lexer_next(Lexer* l, Token* out_token){
    int emit = 0;
    while (1) {
        char c = l->source[l->pos];

        switch (l->state) {
            case LEX_NORMAL:
                if (c == '/'){
                    l->state = LEX_SLASH;
                }
                if (c == '{' || c == '}' || c == '(' || c == ')'  || c == '[' || c == ']' ){
                    out_token->type = TOK_BRACKET;
                    out_token->start = l->pos;
                    out_token->end = l->pos+1;
                    out_token->color = 27; // blue
                    emit = 1;
                }
                if (c == '=' || c == '-' || c == '>' || c == ';' ){
                    out_token->type = TOK_BRACKET;
                    out_token->start = l->pos;
                    out_token->end = l->pos+1;
                    out_token->color = 240;
                    emit = 1;
                }
                if (isalpha(c) || c == '_') {
                    out_token->start = l->pos;
                    l->state = LEX_IDENTIFIER;
                }
                break;
            case LEX_IDENTIFIER:
                if (isalpha(c) || isnumber(c) || c == '_') {
                } else if (c == '(') {
                    out_token->type = TOK_FUNCTION;
                    out_token->end = l->pos;
                    //out_token->color = 20; // blue
                    //out_token->color = 5; // purple
                    out_token->color = 94; // brown
                    emit = 1;
                    l->state = LEX_NORMAL;
                    l->pos--; // reprocess c in normal mode
                } else {
                    out_token->type = TOK_IDENTIFIER;
                    out_token->end = l->pos;
                    out_token->color = 16; // black
                    if (is_keyword(l->source + out_token->start, out_token->end - out_token->start))
                        out_token->color = 13; // pink
                    else if (is_type(l->source + out_token->start, out_token->end - out_token->start))
                        out_token->color = 27; // blue
                    emit = 1;
                    l->state = LEX_NORMAL;
                    l->pos--; // reprocess c in normal mode
                }
                break;
            case LEX_SLASH:
                if (c == '*'){
                    l->state = LEX_SLASH_STAR;
                    out_token->start = l->pos-1;
                }
                if (c == '/'){
                    l->state = LEX_SLASH_SLASH;
                    out_token->start = l->pos-2;
                }
                break;
            case LEX_SLASH_SLASH:
                if (c == '\0'){
                    out_token->type = TOK_COMMENT;
                    out_token->end = l->pos;
                    out_token->color = 28; // green
                    emit = 1;
                    l->state = LEX_NORMAL;
                }
                break;
            case LEX_SLASH_STAR:
                if (c == '*'){
                    l->state = LEX_SLASH_STAR_STAR;
                }
                break;
            case LEX_SLASH_STAR_STAR:
                if (c == '/'){
                    out_token->type = TOK_COMMENT;
                    out_token->end = l->pos+1;
                    out_token->color = 28; // green
                    
                    emit = 1;
                    l->state = LEX_NORMAL;
                }
                break;
        }
        l->pos++;
        if (emit){
            return 1;
        }
        if (c == '\0') return 0;
    }
    return 0;
}
