#ifndef STDARG_H
#define STDARG_H
#include <stdarg-internal.h>

typedef int __builtin_va_list;
typedef __builtin_va_list va_list;

#define va_arg(list, type) __builtin_va_arg(list, sizeof(type))
#endif