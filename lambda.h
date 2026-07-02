#ifndef LAMBDA_H
#define LAMBDA_H

#include <stdint.h>

//typedef void *(*GenericFunc)(void **args, int argc);

typedef struct {
    int magic;
    void (*func)(void *, ...);
    int argc;
    void *args[];
} Lambda;

Lambda *create_lambda(void *func, int argc, ...);
int invoke_lambda(Lambda *l);
int is_lambda(Lambda *l);
void free_lambda(Lambda *l);


#endif
