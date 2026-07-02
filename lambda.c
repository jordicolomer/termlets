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
        void * value = va_arg(args, void*);
        //printf("%d\n", value);
    }

    va_end(args);

    return l;
}


int invoke_lambda(Lambda *l)
{
    //GenericFunc func = (GenericFunc)l->func;
    //return func(l->args, l->argc);
    return 0;
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
