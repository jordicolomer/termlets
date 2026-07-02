#include <stdio.h>
#include <stdlib.h>
#include "lambda.h"
#include <stdarg.h>

#define MAGIC_NUMBER 9281737

Lambda *create_lambda(void *func, int argc, ...)
{
    va_list args;
    va_start(args, argc);

    Lambda *l = malloc(sizeof(Lambda) + sizeof(void*) * argc);
    if (!l)
        return NULL;

    l->magic = MAGIC_NUMBER;
    l->func = func;
    l->argc = argc;
    for (int i = 0; i < argc; i++) {
        l->args[i] = va_arg(args, void*);
    }

    va_end(args);

    return l;
}


int invoke_lambda(Lambda *l)
{
    typedef int (*Func0)(void);
    typedef int (*Func1)(void*);
    typedef int (*Func2)(void*, void*);
    typedef int (*Func3)(void*, void*, void*);
    typedef int (*Func4)(void*, void*, void*, void*);
    typedef int (*Func5)(void*, void*, void*, void*, void*);
    typedef int (*Func6)(void*, void*, void*, void*, void*, void*);
    typedef int (*Func7)(void*, void*, void*, void*, void*, void*, void*);
    typedef int (*Func8)(void*, void*, void*, void*, void*, void*, void*, void*);
    typedef int (*Func9)(void*, void*, void*, void*, void*, void*, void*, void*, void*);
    typedef int (*Func10)(void*, void*, void*, void*, void*, void*, void*, void*, void*, void*);

    switch (l->argc) {
        case 0:
            return ((Func0)l->func)();
        case 1:
            return ((Func1)l->func)(l->args[0]);
        case 2:
            return ((Func2)l->func)(l->args[0], l->args[1]);
        case 3:
            return ((Func3)l->func)(l->args[0], l->args[1], l->args[2]);
        case 4:
            return ((Func4)l->func)(l->args[0], l->args[1], l->args[2], l->args[3]);
        case 5:
            return ((Func5)l->func)(l->args[0], l->args[1], l->args[2], l->args[3], l->args[4]);
        case 6:
            return ((Func6)l->func)(l->args[0], l->args[1], l->args[2], l->args[3], l->args[4], l->args[5]);
        case 7:
            return ((Func7)l->func)(l->args[0], l->args[1], l->args[2], l->args[3], l->args[4], l->args[5], l->args[6]);
        case 8:
            return ((Func8)l->func)(l->args[0], l->args[1], l->args[2], l->args[3], l->args[4], l->args[5], l->args[6], l->args[7]);
        case 9:
            return ((Func9)l->func)(l->args[0], l->args[1], l->args[2], l->args[3], l->args[4], l->args[5], l->args[6], l->args[7], l->args[8]);
        case 10:
            return ((Func10)l->func)(l->args[0], l->args[1], l->args[2], l->args[3], l->args[4], l->args[5], l->args[6], l->args[7], l->args[8], l->args[9]);
        default:
            fprintf(stderr, "invoke_lambda: argc > 10 not supported\n");
            return 0;
    }
}

int is_lambda(Lambda *l)
{
    return l->magic == MAGIC_NUMBER;
}

void free_lambda(Lambda *l)
{
    if (!l)
        return;


    free(l);
}
