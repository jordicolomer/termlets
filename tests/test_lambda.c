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

int main()
{
    int a = 2;
    int b = 3;
    int ret = mul(&a, &b);
    printf("%d\n", ret);

    Lambda * lambda = create_lambda(mul, 2, &a, &b);
    int ret2 = invoke_lambda(lambda);
    printf("%d\n", ret2);
}
