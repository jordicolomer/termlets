#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../lambda.h"


#include <stdarg.h>

int mul(int * a, int * b)
{
    return *a * *b;
}

int int_mul(int a, int b)
{
    return a * b;
}

int main()
{
    int a = 2;
    int b = 3;
    int ret = mul(&a, &b);
    printf("%d\n", ret);

    Lambda * lambda = create_lambda(mul, 2, &a, &b);
    int ret2 = invoke_lambda(lambda);
    printf("%d\n", ret2);
    free_lambda(lambda);

    Lambda * lambda2 = create_lambda(int_mul, 2, 3, 4);
    int ret3 = invoke_lambda(lambda2);
    printf("%d\n", ret3);
    free_lambda(lambda2);    
}
