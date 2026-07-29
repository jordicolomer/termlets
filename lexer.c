#include "lexer.h"

void lexer_init(Lexer* l, const char* source){
    l->source = source;
    l->pos = 0;
    l->state = 0;
    l->finished = 0;
}

int lexer_next(Lexer* l, Token* out_token){
    int emit = 0;
    while (l->source[l->pos] != '\0') {
        char c = l->source[l->pos];

        switch (l->state) {
            case LEX_NORMAL:
                if (c == '/'){
                    l->state = LEX_SLASH;
                }
                break;
            case LEX_SLASH:
                if (c == '*'){
                    l->state = LEX_SLASH_STAR;
                    out_token->start = l->pos-1;
                }
                break;
            case LEX_SLASH_STAR:
                if (c == '*'){
                    l->state = LEX_SLASH_STAR_STAR;
                }
                break;
            case LEX_SLASH_STAR_STAR:
                if (c == '/'){
                    l->state = LEX_NORMAL;
                    out_token->end = l->pos+1;
                    out_token->type = TOK_COMMENT;
                    out_token->color = 252;
                    emit = 1;
                }
                break;
        }
        l->pos++;
        if (emit){
            return 1;
        }
    }
    return 0;
}
