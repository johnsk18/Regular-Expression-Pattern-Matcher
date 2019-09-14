#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define PRINT_DEC(var) printf("%s = %d\n", #var, var);
#define PRINT_STR(var) printf("%s = %s\n", #var, var);
#define OUT printf("[LOG] %s: %s: %d\n", __FILE__, __func__, __LINE__);
#define ERR fprintf(stderr, "[LOG] %s: %s: %d\n", __FILE__, __func__, __LINE__);

#endif