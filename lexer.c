#include "lexer.h"
#include <string.h>

void lexer_init(Lexer* l, const char* source){
    l->source = source;
    l->pos = 0;
    l->state = LEX_BEGIN;
    l->finished = 0;
}

int isalpha(char c){
    return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
}

int isnumber(char c){
    return '0' <= c && c <= '9';
}

/*static const char *keywords[] = {
    "if",
    "else",
    "while",
    "for",
    "return",
    "switch",
    "",   // sentinel
};*/

static const char *keywords[] = {
    /* Control flow */
    "if",
    "else",
    "switch",
    "case",
    "default",
    "while",
    "do",
    "for",

    /* Jump statements */
    "return",
    "break",
    "continue",
    "goto",

    /* Operators / special keywords */
    "sizeof",
    "_Alignof",
    "_Generic",

    /* C11 / C23 */
    "static_assert",
    "alignas",
    "alignof",
    "noreturn",
    "thread_local",

    "",   // sentinel
};

/*static const char *types[] = {
    "int",
    "const",
    "char",
    "long",
    "short",
    "void",
    "size_t",
    "",   // sentinel
};*/

static const char *types[] = {
    /* Fundamental types */
    "void",
    "char",
    "short",
    "int",
    "long",
    "float",
    "double",
    "signed",
    "unsigned",
    "_Bool",
    "_Complex",
    "_Imaginary",

    /* Common typedefs */
    "size_t",
    "ptrdiff_t",
    "intptr_t",
    "uintptr_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "FILE",
    "time_t",
    "clock_t",
    "wchar_t",
    "bool",

    /* Type modifiers / qualifiers */
    "const",
    "volatile",
    "restrict",
    "_Atomic",

    /* Storage class */
    "auto",
    "register",
    "static",
    "extern",
    "typedef",
    "_Thread_local",

    /* Function specifiers */
    "inline",
    "_Noreturn",

    /* User-defined types */
    "struct",
    "union",
    "enum",

    "",   // sentinel
};

int in_list(const char *c, int len, const char *kw[])
{
    for (int i = 0; kw[i][0] != '\0'; i++) {
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
            case LEX_LITERAL_STRING_ESCAPE:
                l->state = LEX_LITERAL_STRING;
                break;
            case LEX_LITERAL_STRING:
                if (c == '\\'){
                    l->state = LEX_LITERAL_STRING_ESCAPE;
                }
                if (c == '"'){
                    out_token->end = l->pos;
                    out_token->color = 125; // red
                    emit = 1;
                    l->state = LEX_NORMAL;
                }
                break;
            case LEX_BEGIN_HASH:
                if (c == '\0'){
                    out_token->end = l->pos;
                    out_token->color = 94; // brown
                    emit = 1;
                    l->state = LEX_BEGIN;
                }
                break;
            case LEX_BEGIN:
                if (c == '#'){
                    l->state = LEX_BEGIN_HASH;
                    out_token->type = TOK_PREPROC;
                    out_token->start = l->pos;
                }
                /* fall through */   // ← Add this comment  
            case LEX_NORMAL:
                if (c == '/'){
                    l->state = LEX_SLASH;
                }
                if (c == '"'){
                    l->state = LEX_LITERAL_STRING;
                    out_token->type = TOK_LITERAL_STRING;
                    out_token->start = l->pos;
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
                if (isnumber(c)) {
                    out_token->type = TOK_LITERAL_NUMBER;
                    out_token->start = l->pos;
                    l->state = LEX_LITERAL_NUMBER;
                }
                break;
            case LEX_LITERAL_NUMBER:
                if (isnumber(c)) {
                } else {
                    out_token->type = TOK_LITERAL_NUMBER;
                    out_token->end = l->pos;
                    out_token->color = 28; // green
                    emit = 1;
                    l->state = LEX_NORMAL;
                    l->pos--; // reprocess character that ended the literal
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
                        out_token->color = 90; // pink
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
                else if (c == '/'){
                    l->state = LEX_SLASH_SLASH;
                    out_token->start = l->pos-1;
                } else {
                    l->state = LEX_NORMAL;
                    l->pos--; // reprocess
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
                if (c == '\0'){
                    out_token->type = TOK_COMMENT;
                    out_token->end = l->pos;
                    out_token->color = 28; // green                    
                    emit = 1;
                }
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
                }else if (c == '*'){
                    l->state = LEX_SLASH_STAR_STAR;
                } else {
                    l->state = LEX_SLASH_STAR;
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
