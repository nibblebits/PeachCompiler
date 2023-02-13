#include <stdarg.h>

int sum(int num, ...)
{
    int result = 0;
    va_list list;
    va_start(list, num);
    
    return result;
}

int main()
{
    return sum(3, 20, 30, 40);
}